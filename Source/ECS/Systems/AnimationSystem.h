#pragma once
#include "entt.hpp"
#include "Resource/Model.h" 

class AnimationSystem
{
public:
    // 更新
    void Update(entt::registry& registry, float elapsedTime);

private:
    // Vectorキーフレームの線形補間
    static DirectX::XMFLOAT3 SampleVector(
        const std::vector<Model::VectorKeyframe>& keyframes,
        float time);

    // Quaternionキーフレームの球面線形補間
    static DirectX::XMFLOAT4 SampleQuaternion(
        const std::vector<Model::QuaternionKeyframe>& keyframes,
        float time);
};