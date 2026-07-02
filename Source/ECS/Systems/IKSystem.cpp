#include "IKSystem.h"
#include "ECS/Systems/TwoBoneIKSolver.h"
#include "Resource/SkeletonUtils.h"

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

void IKSystem::Update(entt::registry& registry)
{
    auto view = registry.view<IKComponent, SkinnedMeshComponent>();
    view.each(
        [&](auto entity, IKComponent& ik, SkinnedMeshComponent& skinned)
        {
            if (!ik.enabled) return;
            if (!skinned.model) return;
            if (skinned.nodeGlobalTransforms.empty()) return;

            SolveLeftFootIK(ik, skinned);
        });
}

void IKSystem::SolveLeftFootIK(IKComponent& ik, SkinnedMeshComponent& skinned)
{
    using namespace DirectX;

    // 決め打ちのノードインデックス(Map_Robot3.gltf の左足)
    constexpr int rootIndex = 74; // thigh_l
    constexpr int midIndex = 75; // calf_l
    constexpr int endIndex = 76; // foot_l

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

    // IKで回転が変わったので、グローバル行列を再計算する
    SkeletonUtils::CalcGlobalTransforms(
        skinned.model->GetNodes(),
        skinned.localPositions, skinned.localRotations, skinned.localScales,
        skinned.nodeGlobalTransforms);

    SkeletonUtils::CalcSkinningMatrices(
        *skinned.model, skinned.nodeGlobalTransforms, skinned.boneTransforms);
}