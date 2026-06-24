#include "MovementSystem.h"

#include "ECS/Components/InputComponent.h"
#include "ECS/Components/MovementComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/PhysicsComponents.h"

#include "entt.hpp"
#include <windows.h>

void MovementSystem::Update(entt::registry& registry, float elapsedTime, const SceneContext& context)
{
    auto view = registry.view<InputComponent, MovementComponent, TransformComponent>();
    view.each([&](auto entity,
        InputComponent& input,
        MovementComponent& movement,
        TransformComponent& transform)
        {
            // カメラの前後左右ベクトル取得 
            DirectX::XMFLOAT3 front = context.camera->GetFront();
            DirectX::XMFLOAT3 right = context.camera->GetRight();

            // XZ平面で正規化し直す
            float frontLen = sqrtf(front.x * front.x + front.z * front.z);
            float rightLen = sqrtf(right.x * right.x + right.z * right.z);

            float frontX = (frontLen > 0.0f) ? front.x / frontLen : 0.0f;
            float frontZ = (frontLen > 0.0f) ? front.z / frontLen : 0.0f;
            float rightX = (rightLen > 0.0f) ? right.x / rightLen : 0.0f;
            float rightZ = (rightLen > 0.0f) ? right.z / rightLen : 0.0f;

            // 移動ベクトルを合成
            float vecX = frontX * input.moveZ + rightX * input.moveX;
            float vecZ = frontZ * input.moveZ + rightZ * input.moveX;

            // 移動ベクトルを正規化して速度をかける
            float vecLen = sqrtf(vecX * vecX + vecZ * vecZ);
            float moveX = 0.0f;
            float moveZ = 0.0f;
            if (vecLen > 0.0f)
            {
                moveX = (vecX / vecLen) * movement.moveSpeed;
                moveZ = (vecZ / vecLen) * movement.moveSpeed;
            }

            // 重力
            movement.velocity.y += movement.gravity * elapsedTime;

            // ジャンプ
            if (input.jump && movement.isGrounded)
            {
                movement.velocity.y = movement.jumpPower;
                movement.isGrounded = false;
            }

            // キャラクターコントローラーを持つ場合は、移動意図をvelocityに渡すだけにする
            if (registry.all_of<CharacterControllerComponent>(entity))
            {
                movement.velocity.x = moveX;
                movement.velocity.z = moveZ;
                return;
            }

            // キャラクターコントローラーを使わない移動
            transform.position.x += moveX * elapsedTime;
            transform.position.z += moveZ * elapsedTime;

            // 垂直移動の反映
            transform.position.y += movement.velocity.y * elapsedTime;

            // 仮の地面判定
            if (transform.position.y <= 0.0f)
            {
                transform.position.y = 0.0f;
                movement.velocity.y = 0.0f;
                movement.isGrounded = true;
            }
        });
}
