#pragma once
#include <entt.hpp>
#include "ECS/Components/AnimatorComponent.h"
#include "ECS/Components/SkinnedMeshComponent.h"
#include "ECS/Components/TransformComponent.h"

class AnimationSystem
{
public:
    // 更新
    void Update(entt::registry& registry, float elapsedTime);

private:
    // キーフレーム補間(位置)
    DirectX::XMFLOAT3 InterpolatePosition(
        const std::vector<Model::VectorKeyframe>& keyframes,
        float time);

    // キーフレーム補間(回転)
    DirectX::XMFLOAT4 InterpolateRotation(
        const std::vector<Model::QuaternionKeyframe>& keyframes,
        float time);

    // キーフレーム補間(サイズ)
    DirectX::XMFLOAT3 InterpolateScale(
        const std::vector<Model::VectorKeyframe>& keyframes,
        float time);

    // グローバルトランスフォーム更新
    void UpdateGlobalTransform(
        std::vector<Model::Node>& nodes,
        int nodeIndex,
        const DirectX::XMFLOAT4X4& parentTransform);
};