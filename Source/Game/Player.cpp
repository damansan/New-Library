#include "Player.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/MeshComponent.h"
#include "ECS/Components/MaterialComponent.h"
#include "ECS/Systems/AnimationSystem.h"
void Player::Initialize(entt::registry& registry, const SceneContext& context)
{
    entity = registry.create();

    auto& t = registry.emplace<TransformComponent>(entity);
    t.position = { -1.0f, 0.0f, 0.0f };
    t.scale = { 1.0f, 1.0f, 1.0f };

    registry.emplace<MeshComponent>(entity, "Data/Model/PlayerRobot/Map_Robot3.gltf");
    //registry.emplace<MeshComponent>(entity, "Data/Model/unitychan.glb");
    registry.emplace<MaterialComponent>(entity).AddPass("pbr");
}

void Player::Update(float elapsedTime, entt::registry& registry, const SceneContext& context)
{
    auto* animator = registry.try_get<AnimatorComponent>(entity);
    if (animator)
    {
        animator->currentAnimation = "Idle";
        animator->loop = true;
        animator->playSpeed = 1.0f;
    }
}

void Player::RenderImGui()
{
}