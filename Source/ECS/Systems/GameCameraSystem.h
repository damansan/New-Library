#pragma once
#include "entt.hpp"
#include "Core/Camera.h"
#include "ECS/Components/GameCameraComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/GameInputComponent.h"

class GameCameraSystem
{
public:
    // ゲームカメラの更新
    void Update(entt::registry& registry, float elapsedTime);

    // Camera クラスに結果を反映
    void ApplyToCamera(entt::registry& registry, Camera& camera);

private:
    // 追従カメラ
    void UpdateFollow(GameCameraComponent& cam, const TransformComponent& target, float elapsedTime);
    // 注視カメラ
    void UpdateLookAt(GameCameraComponent& cam, const TransformComponent& target, float elapsedTime);
    // 固定カメラ
    void UpdateFixed(GameCameraComponent& cam, const TransformComponent& target, float elapsedTime);
};