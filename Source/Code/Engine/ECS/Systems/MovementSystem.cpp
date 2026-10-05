#include "MovementSystem.h"

#include "Engine/ECS/Components/MovementComponent.h"
#include "Engine/ECS/Components/TransformComponent.h"
#include "Engine/ECS/Components/PhysicsComponents.h"

#include "entt.hpp"
#include <windows.h>

void MovementSystem::Update(entt::registry& registry, float elapsedTime, const SceneContext& context)
{
    auto view = registry.view<MovementComponent, TransformComponent>();
    view.each([&](auto entity,
        MovementComponent& movement,
        TransformComponent& transform)
        {
            // 移動がロックされている間は、重力・ジャンプも含めて何もしない
            if (movement.movementLocked) return;

            // 重力
            movement.velocity.y += movement.gravity * elapsedTime;
        });
}
