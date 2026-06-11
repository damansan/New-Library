#include "CameraSystem.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/InputComponent.h"

#include <algorithm>

// カメラ更新
void CameraSystem::Update(entt::registry& registry, float elapsedTime)
{
    auto view = registry.view<CameraComponent, TransformComponent>();
    view.each([&](auto entity, CameraComponent& cam, TransformComponent& transform)
        {
            // 追従対象がいなければスキップ
            if (cam.target == entt::null) return;
            if (!registry.valid(cam.target)) return;

            auto* targetTransform = registry.try_get<TransformComponent>(cam.target);
            if (!targetTransform) return;

            // 初回だけ currentEye と currentTarget を即座に設定
            if (cam.currentEye.x == 0.0f &&
                cam.currentEye.y == 0.0f &&
                cam.currentEye.z == 0.0f)
            {
                cam.currentTarget = targetTransform->position;
                cam.currentEye =
                {
                    targetTransform->position.x,
                    targetTransform->position.y + cam.height,
                    targetTransform->position.z - cam.distance
                };
            }

            // 追従対象のInputComponentからカメラ操作入力を取得
            auto* input = registry.try_get<InputComponent>(cam.target);
            if (input)
            {
                const float rotSpeed = 2.0f;
                cam.yaw += input->cameraX * rotSpeed * elapsedTime;
                cam.pitch += input->cameraY * rotSpeed * elapsedTime;

                // pitch の制限
                cam.pitch = std::clamp(cam.pitch, -0.8f, 1.0f);
            }

            switch (cam.mode)
            {
            case CameraComponent::Mode::Follow:
                UpdateFollow(cam, *targetTransform, elapsedTime);
                break;

            case CameraComponent::Mode::LookAt:
                UpdateLookAt(cam, *targetTransform, elapsedTime);
                break;

            case CameraComponent::Mode::Fixed:
                UpdateFixed(cam, *targetTransform, elapsedTime);
                break;
            }
        });
}

// Cameraクラスに結果を反映
void CameraSystem::ApplyToCamera(entt::registry& registry, Camera& camera)
{
    auto view = registry.view<CameraComponent>();

    for (auto entity : view)
    {
        auto& cam = view.get<CameraComponent>(entity);

        // target が未設定ならスキップ
        if (cam.target == entt::null) return;

        // eye と target が同じ位置ならスキップ
        if (cam.currentEye.x == cam.currentTarget.x &&
            cam.currentEye.y == cam.currentTarget.y &&
            cam.currentEye.z == cam.currentTarget.z) return;

        // Camera クラスに反映
        camera.SetLookAt(
            cam.currentEye,
            cam.currentTarget,
            { 0.0f, 1.0f, 0.0f }
        );

        // 最初に見つかったカメラを使う
        break;
    }
}

// 追従カメラ
void CameraSystem::UpdateFollow(CameraComponent& cam, const TransformComponent& target, float elapsedTime)
{
    // 追従対象の位置
    DirectX::XMFLOAT3 targetPos = target.position;

    // カメラの理想位置を計算
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

    // 滑らかに補間
    float t = 1.0f - powf(0.001f, elapsedTime * cam.followSpeed);

    cam.currentEye.x += (idealEye.x - cam.currentEye.x) * t;
    cam.currentEye.y += (idealEye.y - cam.currentEye.y) * t;
    cam.currentEye.z += (idealEye.z - cam.currentEye.z) * t;

    cam.currentTarget.x += (targetPos.x - cam.currentTarget.x) * t;
    cam.currentTarget.y += ((targetPos.y + cam.height * 0.5f) - cam.currentTarget.y) * t;
    cam.currentTarget.z += (targetPos.z - cam.currentTarget.z) * t;
}

// 注視カメラ
void CameraSystem::UpdateLookAt(CameraComponent& cam, const TransformComponent& target, float elapsedTime)
{
    // 位置固定でターゲットを見る
    cam.currentTarget = target.position;
}

// 固定カメラ
void CameraSystem::UpdateFixed(CameraComponent& cam, const TransformComponent& target, float elapsedTime)
{
    // ターゲットだけ追う
    float t = 1.0f - powf(0.001f, elapsedTime * cam.followSpeed);
    cam.currentTarget.x += (target.position.x - cam.currentTarget.x) * t;
    cam.currentTarget.y += (target.position.y - cam.currentTarget.y) * t;
    cam.currentTarget.z += (target.position.z - cam.currentTarget.z) * t;
}
