#include "IKSystem.h"
#include "TwoBoneIKSolver.h"

#include "ECS/Components/IKComponent.h"
#include "ECS/Components/SkinnedMeshComponent.h"
#include "ECS/Components/TransformComponent.h"

#include "Resource/SkeletonUtils.h"

void IKSystem::Update(entt::registry& registry)
{
	auto view = registry.view<IKComponent, SkinnedMeshComponent, TransformComponent>();

	for (auto entity : view)
	{
		auto& ik = view.get<IKComponent>(entity);

		// 無効、またはボーン名が未設定なら何もしない
		if (!ik.enabled) continue;
		if (ik.rootBoneName.empty() || ik.midBoneName.empty() || ik.endBoneName.empty()) continue;

		auto& skinnedMesh = view.get<SkinnedMeshComponent>(entity);
		auto& transform = view.get<TransformComponent>(entity);
		if (!skinnedMesh.model) continue;

		const auto& nodes = skinnedMesh.model->GetNodes();

		// root/mid/endボーンのインデックスを名前で探す
		int rootIndex = -1;
		int midIndex = -1;
		int endIndex = -1;
		for (int i = 0; i < (int)nodes.size(); i++)
		{
			if (nodes[i].name == ik.rootBoneName) rootIndex = i;
			if (nodes[i].name == ik.midBoneName)  midIndex = i;
			if (nodes[i].name == ik.endBoneName)  endIndex = i;
		}

		// いずれか見つからなければスキップ
		if (rootIndex < 0 || midIndex < 0 || endIndex < 0) continue;

		// 各ノードのローカルpose配列
		std::vector<DirectX::XMFLOAT3> localPositions(nodes.size());
		std::vector<DirectX::XMFLOAT4> localRotations(nodes.size());
		std::vector<DirectX::XMFLOAT3> localScales(nodes.size());

		bool hasPose = (skinnedMesh.localPositions.size() == nodes.size());

		for (int i = 0; i < (int)nodes.size(); i++)
		{
			localPositions[i] = hasPose ? skinnedMesh.localPositions[i] : nodes[i].position;
			localRotations[i] = hasPose ? skinnedMesh.localRotations[i] : nodes[i].rotation;
			localScales[i] = hasPose ? skinnedMesh.localScales[i] : nodes[i].scale;
		}

		// IK適用前のグローバル姿勢を計算
		SkeletonUtils::CalcGlobalTransforms(
			nodes, localPositions, localRotations, localScales,
			skinnedMesh.nodeGlobalTransforms);

		// グローバル行列から位置を取り出す
		auto extractPosition = [](const DirectX::XMFLOAT4X4& m)
			{
				return DirectX::XMFLOAT3(m._41, m._42, m._43);
			};

		// グローバル行列から回転だけを取り出す
		auto extractRotation = [](const DirectX::XMFLOAT4X4& m)
			{
				DirectX::XMVECTOR scale, rotQuat, trans;
				DirectX::XMMatrixDecompose(&scale, &rotQuat, &trans, DirectX::XMLoadFloat4x4(&m));
				DirectX::XMFLOAT4 result;
				DirectX::XMStoreFloat4(&result, rotQuat);
				return result;
			};

		DirectX::XMFLOAT3 rootPos = extractPosition(skinnedMesh.nodeGlobalTransforms[rootIndex]);
		DirectX::XMFLOAT3 midPos = extractPosition(skinnedMesh.nodeGlobalTransforms[midIndex]);
		DirectX::XMFLOAT3 endPos = extractPosition(skinnedMesh.nodeGlobalTransforms[endIndex]);
		DirectX::XMFLOAT4 rootGlobalRotBefore = extractRotation(skinnedMesh.nodeGlobalTransforms[rootIndex]);

		// ワールド空間のtargetを、モデルローカル空間に変換する
		DirectX::XMMATRIX world = transform.GetWorldMatrix();
		DirectX::XMMATRIX worldInv = DirectX::XMMatrixInverse(nullptr, world);

		DirectX::XMVECTOR targetWorld = DirectX::XMLoadFloat3(&ik.targetPosition);

		DirectX::XMFLOAT3 targetLocal;
		DirectX::XMStoreFloat3(&targetLocal, DirectX::XMVector3TransformCoord(targetWorld, worldInv));

		DirectX::XMFLOAT3 poleLocal = midPos;

		// 2BoneIKを解く
		TwoBoneIKSolver::Result result = TwoBoneIKSolver::Solve(
			rootPos, midPos, endPos,
			targetLocal, poleLocal,
			rootGlobalRotBefore);

		DirectX::XMVECTOR newRootGlobalRot = DirectX::XMLoadFloat4(&result.rootGlobalRotation);

		// rootの親の現在のグローバル回転を求める
		int rootParentIndex = nodes[rootIndex].parentIndex;
		DirectX::XMVECTOR parentGlobalRot = (rootParentIndex >= 0)
			? DirectX::XMLoadFloat4(&extractRotation(skinnedMesh.nodeGlobalTransforms[rootParentIndex]))
			: DirectX::XMQuaternionIdentity();

		// root：新しいグローバル回転から、親の回転を打ち消してローカル回転に変換する
		DirectX::XMVECTOR newRootLocalRot = DirectX::XMQuaternionMultiply(
			newRootGlobalRot, DirectX::XMQuaternionInverse(parentGlobalRot));
		DirectX::XMStoreFloat4(&localRotations[rootIndex], newRootLocalRot);

		// mid：rootが回転したことで自動的に引き継がれる向き（inherited）に、
		// 曲げ角度の変化量（bendAxis / bendAngleDelta）を追加してからローカル回転に変換する
		DirectX::XMVECTOR oldMidLocalRot = DirectX::XMLoadFloat4(&localRotations[midIndex]);
		DirectX::XMVECTOR inheritedMidGlobalRot = DirectX::XMQuaternionMultiply(oldMidLocalRot, newRootGlobalRot);

		DirectX::XMVECTOR bendAxis = DirectX::XMLoadFloat3(&result.midBendAxis);
		DirectX::XMVECTOR qBendMid = DirectX::XMQuaternionRotationAxis(bendAxis, result.midBendAngleDelta);

		DirectX::XMVECTOR newMidGlobalRot = DirectX::XMQuaternionMultiply(inheritedMidGlobalRot, qBendMid);
		DirectX::XMVECTOR newMidLocalRot = DirectX::XMQuaternionMultiply(
			newMidGlobalRot, DirectX::XMQuaternionInverse(newRootGlobalRot));
		DirectX::XMStoreFloat4(&localRotations[midIndex], newMidLocalRot);

		// 更新したroot/midのローカル回転で、グローバル姿勢とスキニング行列を計算し直す
		SkeletonUtils::CalcGlobalTransforms(
			nodes, localPositions, localRotations, localScales,
			skinnedMesh.nodeGlobalTransforms);

		SkeletonUtils::CalcSkinningMatrices(
			*skinnedMesh.model, skinnedMesh.nodeGlobalTransforms,
			skinnedMesh.boneTransforms);
	}
}