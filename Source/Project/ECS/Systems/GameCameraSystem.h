#pragma once
#include "entt.hpp"
#include "Engine/Core/Camera.h"
#include "Project/ECS/Components/GameCameraComponent.h"
#include "Engine/ECS/Components/TransformComponent.h"
#include "Project/ECS/Components/GameInputComponent.h"

class GameCameraSystem
{
public:
    // ゲームカメラの更新
    void Update(entt::registry& registry, float elapsedTime);

    // Camera クラスに結果を反映
    void ApplyToCamera(entt::registry& registry, Camera& camera);

private:
	// Follow用のcurrentEye計算
    void UpdateFollowEye(GameCameraComponent& cam, const TransformComponent& followTransform, float elapsedTime);
    // LookAt用のcurrentEye計算
    void UpdateLockOnEye(GameCameraComponent& cam, entt::registry& registry, float elapsedTime);

	// Followカメラの更新
    void UpdateFollowTarget(GameCameraComponent& cam, const TransformComponent& followTransform, float elapsedTime);
    // LookAtカメラの更新
    void UpdateLookAtTarget(GameCameraComponent& cam, entt::registry& registry, float elapsedTime);
};