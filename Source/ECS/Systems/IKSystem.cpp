#include "IKSystem.h"
#include "ECS/Components/IKComponent.h"
#include "ECS/Components/SkinnedMeshComponent.h"
#include "Resource/SkeletonUtils.h"

void IKSystem::Update(entt::registry& registry)
{
	auto view = registry.view<IKComponent, SkinnedMeshComponent>();

	for (auto entity : view)
	{
		auto& ik = view.get<IKComponent>(entity);

		// 無効なら何もしない
		if (!ik.enabled) continue;
		if (ik.targetBoneName.empty()) continue;

		auto& skinnedMesh = view.get<SkinnedMeshComponent>(entity);
		if (!skinnedMesh.model) continue;

		const auto& nodes = skinnedMesh.model->GetNodes();

		// ターゲットボーンのインデックスを名前で探す
		int targetIndex = -1;
		for (int i = 0; i < (int)nodes.size(); i++)
		{
			if (nodes[i].name == ik.targetBoneName)
			{
				targetIndex = i;
				break;
			}
		}

		// 見つからなければスキップ
		if (targetIndex < 0) continue;

		// 各ノードのローカルpose配列をモデルのデフォルト値で初期化
		std::vector<DirectX::XMFLOAT3> localPositions(nodes.size());
		std::vector<DirectX::XMFLOAT4> localRotations(nodes.size());
		std::vector<DirectX::XMFLOAT3> localScales(nodes.size());

		for (int i = 0; i < (int)nodes.size(); i++)
		{
			localPositions[i] = nodes[i].position;
			localRotations[i] = nodes[i].rotation;
			localScales[i] = nodes[i].scale;
		}

		// ターゲットノードだけ、manualの値で上書き
		localPositions[targetIndex].x += ik.manualPosition.x;
		localPositions[targetIndex].y += ik.manualPosition.y;
		localPositions[targetIndex].z += ik.manualPosition.z;

		// オイラー角 → クォータニオンに変換してから上書き
		DirectX::XMVECTOR q = DirectX::XMQuaternionRotationRollPitchYaw(
			ik.manualRotationEuler.x,
			ik.manualRotationEuler.y,
			ik.manualRotationEuler.z);
		DirectX::XMStoreFloat4(&localRotations[targetIndex], q);

		// 階層計算 → スキニング行列化
		std::vector<DirectX::XMFLOAT4X4> globalTransforms;
		SkeletonUtils::CalcGlobalTransforms(
			nodes, localPositions, localRotations, localScales,
			globalTransforms);

		SkeletonUtils::CalcSkinningMatrices(
			*skinnedMesh.model, globalTransforms,
			skinnedMesh.boneTransforms);
	}
}