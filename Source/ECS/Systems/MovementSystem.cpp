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
            // 移動がロックされている間は、重力・ジャンプも含めて何もしない
            if (movement.movementLocked) return;

            // 重力
            movement.velocity.y += movement.gravity * elapsedTime;

            // ジャンプ
            if (input.jump && movement.isGrounded)
            {
                movement.velocity.y = movement.jumpPower;
                movement.isGrounded = false;
            }

            // キャラクターコントローラーを持つ場合は、PlayerMoveSystem/PhysicsSystemに任せる
            if (registry.all_of<CharacterControllerComponent>(entity))
            {
                return;
            }

            // キャラクターコントローラーを使わない場合のみ、ここで直接位置を更新する
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
