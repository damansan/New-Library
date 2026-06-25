#include "PhysicsSystem.h"
#include "Core/PhysicsDestroyQueue.h"
#include "Core/PhysicsFilter.h"

#include "ECS/Components/PhysicsComponents.h"
#include "ECS/Components/StaticMeshComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/MovementComponent.h"

#include "Resource/Model.h"

#include <vector>

using namespace physx;

// TransformComponentの回転からPxQuatを作る
static PxQuat ToPxQuat(const DirectX::XMFLOAT3& rotation)
{
	DirectX::XMVECTOR q = DirectX::XMQuaternionRotationRollPitchYaw(
		rotation.x, rotation.y, rotation.z);
	DirectX::XMFLOAT4 qf;
	DirectX::XMStoreFloat4(&qf, q);
	return PxQuat(qf.x, qf.y, qf.z, qf.w);
}

// サンプリング座標作成
static void GenerateFootSamplePoints(
	const PxExtendedVec3& footPosition,
	float radius,
	int sampleCount,
	std::vector<PxVec3>& outPoints)
{
	outPoints.clear();
	outPoints.reserve(sampleCount + 1);

	const float centerX = static_cast<float>(footPosition.x);
	const float centerY = static_cast<float>(footPosition.y);
	const float centerZ = static_cast<float>(footPosition.z);

	// 中心点
	outPoints.emplace_back(centerX, centerY, centerZ);

	// 円上の点
	for (int i = 0; i < sampleCount; ++i)
	{
		const float angle = (2.0f * PxPi * static_cast<float>(i)) / static_cast<float>(sampleCount);
		const float x = centerX + radius * std::cosf(angle);
		const float z = centerZ + radius * std::sinf(angle);
		outPoints.emplace_back(x, centerY, z);
	}
}

// 更新
void PhysicsSystem::Update(entt::registry& registry, PhysX& physX, physx::PxScene* pxScene, physx::PxControllerManager* controllerManager, float elapsedTime)
{
	ProcessDestroyQueue();
	ProcessStaticBodies(registry, physX, pxScene);
	// ProcessDynamicBodies(registry, physX, pxScene);
	ProcessCharacterControllers(registry, physX, controllerManager, elapsedTime);
}

// 削除予約されたActorを解放する
void PhysicsSystem::ProcessDestroyQueue()
{
	auto& destroyQueue = PhysicsDestroyQueue::GetQueue();
	for (PxActor* actor : destroyQueue)
	{
		if (!actor) continue;
		if (actor->getScene())
		{
			actor->getScene()->removeActor(*actor);
		}
		actor->release();
	}
	destroyQueue.clear();

	// Controllerの解放
	auto& controllerDestroyQueue = PhysicsDestroyQueue::GetControllerQueue();
	for (PxController* controller : controllerDestroyQueue)
	{
		if (!controller) continue;
		controller->release();
	}
	controllerDestroyQueue.clear();
}

// 未生成のStaticBodyComponent生成
void PhysicsSystem::ProcessStaticBodies(entt::registry& registry, PhysX& physX, physx::PxScene* pxScene)
{
	auto view = registry.view<StaticBodyComponent, StaticMeshComponent, TransformComponent>();

	for (auto entity : view)
	{
		auto& body = view.get<StaticBodyComponent>(entity);

		// 既に生成済みならスキップ
		if (body.actor != nullptr) continue;

		auto& staticMesh = view.get<StaticMeshComponent>(entity);
		auto& transform = view.get<TransformComponent>(entity);

		// モデルが未ロードならスキップ
		if (!staticMesh.model) continue;

		// Actorの位置・回転を設定
		PxTransform pxTransform(
			PxVec3(transform.position.x, transform.position.y, transform.position.z),
			ToPxQuat(transform.rotation)
		);

		PxRigidStatic* actor = physX.GetPhysics()->createRigidStatic(pxTransform);

		// モデル内の各Meshごとにシェイプを追加
		int shapeCount = 0;
		for (const auto& mesh : staticMesh.model->GetMeshes())
		{
			if (mesh.vertices.empty() || mesh.indices.empty()) continue;

			PxTriangleMesh* triMesh = physX.CreateTriangleMesh(
				mesh.vertices.data(),
				static_cast<uint32_t>(mesh.vertices.size()),
				sizeof(Model::Vertex),
				mesh.indices.data(),
				static_cast<uint32_t>(mesh.indices.size() / 3)
			);

			if (!triMesh) continue; // メッシュ生成失敗

			PxMeshScale meshScale(
				PxVec3(transform.scale.x, transform.scale.y, transform.scale.z)
			);
			PxTriangleMeshGeometry geometry(triMesh, meshScale);

			PxShape* shape = PxRigidActorExt::createExclusiveShape(
				*actor, geometry, *physX.GetDefaultMaterial()
			);

			triMesh->release();

			if (shape) shapeCount++;
		}

		// シェイプが1つも作れなかった場合はActorごと破棄
		if (shapeCount == 0)
		{
			actor->release();
			continue;
		}

		pxScene->addActor(*actor);
		body.actor = actor;
	}
}

// 未生成のCharacterControllerComponent生成+CharacterController移動処理
void PhysicsSystem::ProcessCharacterControllers(entt::registry& registry, PhysX& physX, physx::PxControllerManager* controllerManager, float elapsedTime)
{
	auto view = registry.view<CharacterControllerComponent, TransformComponent>();

	for (auto entity : view)
	{
		auto& ctrl = view.get<CharacterControllerComponent>(entity);
		auto& transform = view.get<TransformComponent>(entity);

		// 未生成ならController本体を生成する
		if (ctrl.controller == nullptr)
		{
			PxCapsuleControllerDesc desc;
			desc.radius = ctrl.radius;
			desc.height = ctrl.height;
			desc.position = PxExtendedVec3(transform.position.x, transform.position.y, transform.position.z);
			desc.material = physX.GetDefaultMaterial();
			desc.upDirection = PxVec3(0.0f, 1.0f, 0.0f);
			desc.contactOffset = 0.01f;

			if (!desc.isValid()) continue; // パラメータ不正なら生成しない

			ctrl.controller = controllerManager->createController(desc);

			// 生成直後のフレームは移動させない
			continue;
		}

		// MovementComponentが無いEntityは移動させない
		auto* movement = registry.try_get<MovementComponent>(entity);

		PxVec3 disp(0.0f, 0.0f, 0.0f);
		if (movement)
		{
			disp = PxVec3(
				movement->velocity.x * elapsedTime,
				movement->velocity.y * elapsedTime,
				movement->velocity.z * elapsedTime
			);
		}

		// 移動を実行
		PxControllerFilters filters;
		PxControllerCollisionFlags collisionFlags = ctrl.controller->move(disp, 0.001f, elapsedTime, filters);

		// 結果の位置をTransformへ反映
		PxExtendedVec3 foot = ctrl.controller->getFootPosition();
		transform.position.x = static_cast<float>(foot.x);
		transform.position.y = static_cast<float>(foot.y);
		transform.position.z = static_cast<float>(foot.z);

		// 接地判定をMovementComponentへ反映
		if (movement)
		{
			bool grounded = (collisionFlags & PxControllerCollisionFlag::eCOLLISION_DOWN);
			movement->isGrounded = grounded;

			// わずかな下方速度を残して斜面に追従させる
			constexpr float groundStickVelocity = -2.0f;
			if (grounded && movement->velocity.y < 0.0f)
			{
				movement->velocity.y = groundStickVelocity;
			}
		}
	}
}

// 足元判定サポート
void PhysicsSystem::ProcessEdgeSlide(entt::registry& registry, physx::PxScene* pxScene)
{
}

namespace
{
	// Walkable属性を持つシェイプのみ判定対象にする
	class WalkableQueryFilter : public PxQueryFilterCallback
	{
	public:
		PxQueryHitType::Enum preFilter(
			const PxFilterData& filterData, const PxShape* shape,
			const PxRigidActor* actor, PxHitFlags& queryFlags) override
		{
			// トリガーシェイプは地面として扱わない
			if (shape->getFlags() & PxShapeFlag::eTRIGGER_SHAPE)
			{
				return PxQueryHitType::eNONE;
			}

			// Walkable属性が立っていないシェイプは無視（自キャラ自身のカプセルもここで除外される）
			PxFilterData queryFilterData = shape->getQueryFilterData();
			if (!(queryFilterData.word2 & static_cast<PxU32>(PhysicsAttribute::Walkable)))
			{
				return PxQueryHitType::eNONE;
			}

			return PxQueryHitType::eBLOCK;
		}

		PxQueryHitType::Enum postFilter(
			const PxFilterData& filterData, const PxQueryHit& hit,
			const PxShape* shape, const PxRigidActor* actor) override
		{
			return PxQueryHitType::eBLOCK;
		}
	};
}

// 1点について、下方向にWalkableな地面があるかを判定する
static bool IsPointSupported(PxScene* pxScene, const PxVec3& point, float probeDistance)
{
	PxRaycastBuffer hitBuffer;
	WalkableQueryFilter filterCallback;

	PxQueryFilterData filterData;
	filterData.flags |= PxQueryFlag::ePREFILTER;

	return pxScene->raycast(
		point,
		PxVec3(0.0f, -1.0f, 0.0f),
		probeDistance,
		hitBuffer,
		PxHitFlag::eDEFAULT,
		filterData,
		&filterCallback
	);
}