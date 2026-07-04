#pragma once
#include <DirectXMath.h>
#include "entt.hpp"

#include "ECS/Components/IKComponent.h"
#include "ECS/Components/SkinnedMeshComponent.h"
#include "ECS/Components/TransformComponent.h"

namespace physx { class PxScene; }

class IKSystem
{
public:
    // 更新(AnimationSystemの後、MeshLoaderSystemの前に呼ぶ想定)
    void Update(entt::registry& registry, physx::PxScene* pxScene);

private:
    // 1体分のFootIK処理(左足決め打ち・確認用)
    void SolveLeftFootIK(IKComponent& ik, SkinnedMeshComponent& skinned, physx::PxScene* pxScene, const DirectX::XMFLOAT4X4& entityWorldMatrix);
};