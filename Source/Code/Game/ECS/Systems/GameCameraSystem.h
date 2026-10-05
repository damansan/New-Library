#pragma once
#include "entt.hpp"
#include "Engine/Core/Camera.h"
#include "Game/ECS/Components/GameCameraComponent.h"
#include "Engine/ECS/Components/TransformComponent.h"
#include "Game/ECS/Components/GameInputComponent.h"

#include "Engine/Rendering/ShapeRenderer.h"

class GameCameraSystem
{
public:
    void Initialize(ID3D11Device* device);

    // ゲームカメラの更新
    void Update(entt::registry& registry, float elapsedTime);

    // Camera クラスに結果を反映
    void ApplyToCamera(entt::registry& registry, Camera& camera);

    // デバッグ描画
    void DrawDebug(
        entt::registry& registry,
        ID3D11DeviceContext* dc,
        const DirectX::XMFLOAT4X4& view,
        const DirectX::XMFLOAT4X4& projection,
        const DirectX::XMFLOAT3& lightDirection);

private:
    ShapeRenderer shapeRenderer;

    // Followカメラの更新
    void UpdateFollowTarget(GameCameraComponent& cam, const TransformComponent& followTransform, float elapsedTime);
	// Follow用のcurrentEye計算
    void UpdateFollowEye(GameCameraComponent& cam, const TransformComponent& followTransform, float elapsedTime);

    // LookAtカメラの更新
    void UpdateLookAtTarget(GameCameraComponent& cam, entt::registry& registry, float elapsedTime);
    // LookAt用のcurrentEye計算
    void UpdateLockOnEye(GameCameraComponent& cam, entt::registry& registry, float elapsedTime);
};