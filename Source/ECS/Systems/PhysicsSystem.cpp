#include "PhysicsSystem.h"
#include "Core/PhysicsDestroyQueue.h"
#include "ECS/Components/PhysicsComponents.h"
#include "ECS/Components/StaticMeshComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/MovementComponent.h"
#include "Resource/Model.h"

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
	auto& controllerQueue = PhysicsDestroyQueue::GetControllerQueue();
	for (PxController* controller : controllerQueue)
	{
		if (!controller) continue;
		controller->release();
	}
	controllerQueue.clear();
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

			if (!desc.isValid()) continue; // パラメータ不正なら生成しない

			ctrl.controller = controllerManager->createController(desc);

			// 生成直後のフレームは移動させない（次フレームから移動を反映する）
			continue;
		}

		// MovementComponentが無いEntityは移動させない（その場で待機）
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

		// 移動を実行（壁・段差・地面との衝突解決はPhysXが行う）
		PxControllerFilters filters;
		PxControllerCollisionFlags collisionFlags = ctrl.controller->move(disp, 0.001f, elapsedTime, filters);

		// 結果の位置をTransformへ反映（足元座標）
		PxExtendedVec3 foot = ctrl.controller->getFootPosition();
		transform.position.x = static_cast<float>(foot.x);
		transform.position.y = static_cast<float>(foot.y);
		transform.position.z = static_cast<float>(foot.z);

		// 接地判定をMovementComponentへ反映
		if (movement)
		{
			bool grounded = (collisionFlags & PxControllerCollisionFlag::eCOLLISION_DOWN);
			movement->isGrounded = grounded;

			// 着地した瞬間に下方向の速度が残っているとめり込み判定が起きるためリセット
			if (grounded && movement->velocity.y < 0.0f)
			{
				movement->velocity.y = 0.0f;
			}
		}
	}
}
