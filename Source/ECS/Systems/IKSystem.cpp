#include "IKSystem.h"
#include "ECS/Systems/TwoBoneIKSolver.h"
#include "Resource/SkeletonUtils.h"
#include "ECS/Systems/PhysicsSystem.h"
#include "Core/PhysicsLayer.h"

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

			SolveLeftFootIK(ik, skinned, pxScene, entityWorldMatrix);
		});
}

void IKSystem::SolveLeftFootIK(IKComponent& ik, SkinnedMeshComponent& skinned, physx::PxScene* pxScene, const DirectX::XMFLOAT4X4& entityWorldMatrix)
{
	using namespace DirectX;

	// 決め打ちのノードインデックス(Map_Robot3.gltf の左足)
	constexpr int rootIndex = 74; // thigh_l
	constexpr int midIndex = 75;  // calf_l
	constexpr int endIndex = 76;  // foot_l

	auto& globals = skinned.nodeGlobalTransforms;
	if (rootIndex >= (int)globals.size() ||
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
	if (!ik.hasInitializedTarget)
	{
		ik.targetPos = endPos;
		ik.hasInitializedTarget = true;
	}

	XMFLOAT3 targetPos = ik.targetPos;

	// 足首の少し上からまっすぐ下へレイを飛ばし、地面の法線を取得する
		// endPosはモデル空間の座標なので、レイを飛ばす前にワールド空間へ変換する
	XMVECTOR endPosModel = XMLoadFloat3(&endPos);
	XMMATRIX entityWorld = XMLoadFloat4x4(&entityWorldMatrix);
	XMVECTOR endPosWorldV = XMVector3TransformCoord(endPosModel, entityWorld);

	XMFLOAT3 rayOrigin;
	XMStoreFloat3(&rayOrigin, endPosWorldV);
	rayOrigin.y += 0.2f; // 地面にめり込んで判定漏れしないよう少し持ち上げる
	XMFLOAT3 rayDirection = { 0.0f, -1.0f, 0.0f };
	constexpr float rayMaxDistance = 0.5f;

	RaycastHitResult groundHit = PhysicsSystem::Raycast(
		pxScene, rayOrigin, rayDirection, rayMaxDistance, PhysicsLayer::Static);

	if (groundHit.hit)
	{
		// 足首から足裏までの高さ分、地面の当たり位置より持ち上げる(めり込み防止)
		// ※仮値。実際のモデルの足の厚みに合わせて微調整が必要
		constexpr float ankleHeight = 0.1f;
		XMFLOAT3 hitPos = groundHit.position;
		hitPos.y += ankleHeight;

		// groundHit.positionはワールド座標なので、モデル空間に変換してからtargetPosとして使う
		// (地面が見つからなかった場合は、targetPosは最初に代入したik.targetPosのまま)
		XMVECTOR hitPosWorldV = XMLoadFloat3(&hitPos);
		XMMATRIX entityWorldInverse = XMMatrixInverse(nullptr, entityWorld);
		XMVECTOR hitPosModelV = XMVector3TransformCoord(hitPosWorldV, entityWorldInverse);
		XMStoreFloat3(&targetPos, hitPosModelV);
	}

	// poleVector: 膝を前方(モデルのZ+方向を仮定)に向ける
	XMFLOAT3 poleVector = rootPos;
	poleVector.z -= 1.0f;

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