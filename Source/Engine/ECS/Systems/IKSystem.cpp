#include "IKSystem.h"
#include "Engine/ECS/Systems/TwoBoneIKSolver.h"
#include "Engine/Resource/SkeletonUtils.h"
#include "Engine/ECS/Systems/PhysicsSystem.h"
#include "Engine/Core/PhysicsLayer.h"

namespace
{
	using namespace DirectX;

	// ワールド回転を、親のワールド回転を基準にしたローカル回転に変換する
	XMVECTOR WorldToLocalRotation(FXMVECTOR worldRotation, FXMVECTOR parentWorldRotation)
	{
		XMVECTOR parentInverse = XMQuaternionInverse(parentWorldRotation);
		// worldRotation = localRotation * parentWorldRotation なので
		// localRotation = worldRotation * (parentWorldRotation)^-1
		return XMQuaternionMultiply(worldRotation, parentInverse);
	}

	// XMFLOAT4X4(ワールド行列)から、位置と回転(クォータニオン)を取り出す
	void DecomposePosRot(const XMFLOAT4X4& world, XMFLOAT3& outPos, XMFLOAT4& outRot)
	{
		XMMATRIX m = XMLoadFloat4x4(&world);

		XMVECTOR scale, rotQuat, trans;
		XMMatrixDecompose(&scale, &rotQuat, &trans, m);

		XMStoreFloat3(&outPos, trans);
		XMStoreFloat4(&outRot, rotQuat);
	}
}

void IKSystem::Update(entt::registry& registry, physx::PxScene* pxScene)
{
	auto view = registry.view<IKComponent, SkinnedMeshComponent, TransformComponent>();
	view.each(
		[&](auto entity, IKComponent& ik, SkinnedMeshComponent& skinned, TransformComponent& transform)
		{
			if (!ik.enabled) return;
			if (!skinned.model) return;
			if (skinned.nodeGlobalTransforms.empty()) return;

			DirectX::XMFLOAT4X4 entityWorldMatrix;
			DirectX::XMStoreFloat4x4(&entityWorldMatrix, transform.GetWorldMatrix());

			for (auto& footSettings : ik.feet)
			{
				SolveFootIK(footSettings, skinned, pxScene, entityWorldMatrix);
			}
		});
}

// 1本の足のFootIK処理
void IKSystem::SolveFootIK(FootIKSettings& footSettings, SkinnedMeshComponent& skinned, physx::PxScene* pxScene, const DirectX::XMFLOAT4X4& entityWorldMatrix)
{
	using namespace DirectX;

	// ボーンインデックス
	const int rootIndex = footSettings.rootBoneIndex;
	const int midIndex = footSettings.midBoneIndex;
	const int endIndex = footSettings.endBoneIndex;

	auto& globals = skinned.nodeGlobalTransforms;
	if (rootIndex < 0 || midIndex < 0 || endIndex < 0 ||
		rootIndex >= (int)globals.size() ||
		midIndex >= (int)globals.size() ||
		endIndex >= (int)globals.size())
	{
		return;
	}

	// 現在のワールド位置・回転を取り出す
	XMFLOAT3 rootPos, midPos, endPos;
	XMFLOAT4 rootRot, midRot, endRot;
	DecomposePosRot(globals[rootIndex], rootPos, rootRot);
	DecomposePosRot(globals[midIndex], midPos, midRot);
	DecomposePosRot(globals[endIndex], endPos, endRot);

	// 初回だけ、現在の足首位置をtargetPosの初期値として使う
	if (!footSettings.hasInitializedTarget)
	{
		footSettings.targetPos = endPos;
		footSettings.hasInitializedTarget = true;
	}

	XMFLOAT3 targetPos = footSettings.targetPos;

	// 足首の少し上からまっすぐ下へレイを飛ばし、地面の法線を取得する
		// endPosはモデル空間の座標なので、レイを飛ばす前にワールド空間へ変換する
	XMVECTOR endPosModel = XMLoadFloat3(&endPos);
	XMMATRIX entityWorld = XMLoadFloat4x4(&entityWorldMatrix);
	XMVECTOR endPosWorldV = XMVector3TransformCoord(endPosModel, entityWorld);

	XMFLOAT3 rayOrigin;
	XMStoreFloat3(&rayOrigin, endPosWorldV);
	rayOrigin.y += footSettings.rayStartOffset; // めり込まないように少し上げる
	XMFLOAT3 rayDirection = { 0.0f, -1.0f, 0.0f };
	float rayMaxDistance = footSettings.rayMaxDistance;

	// レイを飛ばす
	RaycastHitResult groundHit = PhysicsSystem::Raycast(
		pxScene, rayOrigin, rayDirection, rayMaxDistance, PhysicsLayer::Static);

	if (groundHit.hit)
	{
		// 地面から足首までの高さ分、地面の当たり位置より持ち上げる(めり込み防止)
		XMFLOAT3 hitPos = groundHit.position;
		hitPos.y += footSettings.ankleHeight;

		// groundHit.positionはワールド座標なので、モデル空間に変換してからtargetPosとして使う
		// (地面が見つからなかった場合は、targetPosは最初に代入したfootSettings.targetPosのまま)
		XMVECTOR hitPosWorldV = XMLoadFloat3(&hitPos);
		XMMATRIX entityWorldInverse = XMMatrixInverse(nullptr, entityWorld);
		XMVECTOR hitPosModelV = XMVector3TransformCoord(hitPosWorldV, entityWorldInverse);
		XMStoreFloat3(&targetPos, hitPosModelV);
	}

	// poleVector: 膝を前方(モデルのZ+方向を仮定)に向ける
	XMFLOAT3 poleVector = rootPos;
	poleVector.z += footSettings.poleVectorForward ? -1.0f : 1.0f;

	// 2BoneIKを解く
	TwoBoneIKSolver::Result result = TwoBoneIKSolver::Solve(
		rootPos, midPos, endPos,
		rootRot, midRot,
		targetPos, poleVector);

	// ワールド回転 → ローカル回転 に変換して書き戻す
	// rootの親(pelvis)のワールド回転はまだ変わっていないので、そのまま使える
	XMVECTOR pelvisWorldRot;
	{
		XMFLOAT3 dummyPos;
		XMFLOAT4 pelvisRot;
		int pelvisIndex = skinned.model->GetNodes()[rootIndex].parentIndex;
		DecomposePosRot(globals[pelvisIndex], dummyPos, pelvisRot);
		pelvisWorldRot = XMLoadFloat4(&pelvisRot);
	}

	XMVECTOR newRootLocal = WorldToLocalRotation(
		XMLoadFloat4(&result.rootRotation), pelvisWorldRot);
	XMVECTOR newMidLocal = WorldToLocalRotation(
		XMLoadFloat4(&result.midRotation), XMLoadFloat4(&result.rootRotation));

	XMStoreFloat4(&skinned.localRotations[rootIndex], newRootLocal);
	XMStoreFloat4(&skinned.localRotations[midIndex], newMidLocal);

	// 足首: 地面の法線に合わせて、水平からのズレ分だけ元の向き(endRot)に上乗せする
	XMVECTOR groundUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMVECTOR groundDeltaRotation = groundHit.hit
		? TwoBoneIKSolver::RotationFromToRotation(groundUp, XMLoadFloat3(&groundHit.normal))
		: XMQuaternionIdentity(); // 地面が見つからなければ、元の向きのまま何もしない

	XMVECTOR newEndWorld = XMQuaternionMultiply(XMLoadFloat4(&endRot), groundDeltaRotation);
	XMVECTOR newEndLocal = WorldToLocalRotation(
		newEndWorld, XMLoadFloat4(&result.midRotation));

	XMStoreFloat4(&skinned.localRotations[endIndex], newEndLocal);

	// IKで回転が変わったので、グローバル行列を再計算する
	SkeletonUtils::CalcGlobalTransforms(
		skinned.model->GetNodes(),
		skinned.localPositions, skinned.localRotations, skinned.localScales,
		skinned.nodeGlobalTransforms);

	SkeletonUtils::CalcSkinningMatrices(
		*skinned.model, skinned.nodeGlobalTransforms, skinned.boneTransforms);
}