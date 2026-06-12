#include "GameCameraSystem.h"
#include <algorithm>

void GameCameraSystem::Update(entt::registry& registry, float elapsedTime)
{
    auto view = registry.view<GameCameraComponent, TransformComponent>();
    view.each([&](auto entity, GameCameraComponent& cam, TransformComponent& transform)
        {
            // 追従対象チェック
            if (cam.target == entt::null) return;
            if (!registry.valid(cam.target)) return;

            auto* targetTransform = registry.try_get<TransformComponent>(cam.target);
            if (!targetTransform) return;

            // 初回だけ currentEye / currentTarget を即座に設定
            if (cam.base.currentEye.x == 0.0f &&
                cam.base.currentEye.y == 0.0f &&
                cam.base.currentEye.z == 0.0f)
            {
                cam.base.currentTarget = targetTransform->position;
                cam.base.currentEye =
                {
                    targetTransform->position.x,
                    targetTransform->position.y + cam.height,
                    targetTransform->position.z - cam.distance
                };
            }

            // InputComponent からカメラ操作入力を取得
            auto* input = registry.try_get<InputComponent>(cam.target);
            if (input)
            {
                const float rotSpeed = 2.0f;
                cam.yaw += input->cameraX * rotSpeed * elapsedTime;
                cam.pitch += input->cameraY * rotSpeed * elapsedTime;
                cam.pitch = std::clamp(cam.pitch, -0.8f, 1.0f);
            }

            switch (cam.mode)
            {
            case GameCameraComponent::Mode::Follow:
                UpdateFollow(cam, *targetTransform, elapsedTime);
                break;
            case GameCameraComponent::Mode::LookAt:
                UpdateLookAt(cam, *targetTransform, elapsedTime);
                break;
            case GameCameraComponent::Mode::Fixed:
                UpdateFixed(cam, *targetTransform, elapsedTime);
                break;
            }
        });
}

void GameCameraSystem::ApplyToCamera(entt::registry& registry, Camera& camera)
{
    auto view = registry.view<GameCameraComponent>();
    for (auto entity : view)
    {
        auto& cam = view.get<GameCameraComponent>(entity);

        // target が未設定ならスキップ
        if (cam.target == entt::null) return;

        // eye と target が同じ位置ならスキップ
        if (cam.base.currentEye.x == cam.base.currentTarget.x &&
            cam.base.currentEye.y == cam.base.currentTarget.y &&
            cam.base.currentEye.z == cam.base.currentTarget.z) return;

        camera.SetLookAt(
            cam.base.currentEye,
            cam.base.currentTarget,
            { 0.0f, 1.0f, 0.0f }
        );
        // 最初の1つだけ反映
        break;
    }
}

// 追従カメラ
void GameCameraSystem::UpdateFollow(
    GameCameraComponent& cam,
    const TransformComponent& target,
    float elapsedTime)
{
    DirectX::XMFLOAT3 targetPos = target.position;

    float sinYaw = sinf(cam.yaw);
    float cosYaw = cosf(cam.yaw);
    float cosPitch = cosf(cam.pitch);
    float sinPitch = sinf(cam.pitch);

    DirectX::XMFLOAT3 idealEye =
    {
        targetPos.x - sinYaw * cosPitch * cam.distance,
        targetPos.y + sinPitch * cam.distance + cam.height,
        targetPos.z - cosYaw * cosPitch * cam.distance
    };

    float t = 1.0f - powf(0.001f, elapsedTime * cam.followSpeed);

    cam.base.currentEye.x += (idealEye.x - cam.base.currentEye.x) * t;
    cam.base.currentEye.y += (idealEye.y - cam.base.currentEye.y) * t;
    cam.base.currentEye.z += (idealEye.z - cam.base.currentEye.z) * t;

    cam.base.currentTarget.x += (targetPos.x - cam.base.currentTarget.x) * t;
    cam.base.currentTarget.y += ((targetPos.y + cam.height * 0.5f) - cam.base.currentTarget.y) * t;
    cam.base.currentTarget.z += (targetPos.z - cam.base.currentTarget.z) * t;
}

// 注視カメラ
void GameCameraSystem::UpdateLookAt(
    GameCameraComponent& cam,
    const TransformComponent& target,
    float elapsedTime)
{
    cam.base.currentTarget = target.position;
}

// 固定カメラ
void GameCameraSystem::UpdateFixed(
    GameCameraComponent& cam,
    const TransformComponent& target,
    float elapsedTime)
{
    float t = 1.0f - powf(0.001f, elapsedTime * cam.followSpeed);
    cam.base.currentTarget.x += (target.position.x - cam.base.currentTarget.x) * t;
    cam.base.currentTarget.y += (target.position.y - cam.base.currentTarget.y) * t;
    cam.base.currentTarget.z += (target.position.z - cam.base.currentTarget.z) * t;
}