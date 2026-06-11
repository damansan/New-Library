#pragma once
#include "entt.hpp"
#include "Core/Camera.h"
#include "ECS/Components/CameraComponent.h"
#include "ECS/Components/TransformComponent.h"

class CameraSystem
{
public:
    // カメラ更新
    void Update(entt::registry& registry, float elapsedTime);

    // Cameraクラスに結果を反映
    void ApplyToCamera(entt::registry& registry, Camera& camera);

private:
    // 追従カメラ
    void UpdateFollow(CameraComponent& cam, const TransformComponent& target, float elapsedTime);
    // 注視カメラ
    void UpdateLookAt(CameraComponent& cam, const TransformComponent& target, float elapsedTime);
    // 固定カメラ
    void UpdateFixed(CameraComponent& cam, const TransformComponent& target, float elapsedTime);
};