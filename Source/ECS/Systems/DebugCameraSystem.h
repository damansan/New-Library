#pragma once
#include "entt.hpp"
#include "Core/Camera.h"
#include "ECS/Components/FreeCameraComponent.h"

class DebugCameraSystem
{
public:
    // フリーカメラの更新
    void Update(entt::registry& registry, float elapsedTime);

    // Camera クラスに結果を反映
    void ApplyToCamera(entt::registry& registry, Camera& camera);

private:
    // フリーカメラ更新処理
    void UpdateFreeCamera(FreeCameraComponent& free, float elapsedTime);
};