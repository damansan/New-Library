#include "Enemy.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/MeshComponent.h"
#include "ECS/Components/MaterialComponent.h"
#include "ECS/Components/AnimatorComponent.h"

void Enemy::Initialize(entt::registry& registry, const SceneContext& context)
{
    entity = registry.create();

    auto& t = registry.emplace<TransformComponent>(entity);
    t.position = { 0.0f, 0.0f, 0.0f };
    t.scale = { 1.0f, 1.0f, 1.0f };

    registry.emplace<MeshComponent>(entity, "Data/Model/EnemyRobot/SK_SteamPunkBoss.gltf");
    //registry.emplace<MeshComponent>(entity, "Data/Model/unitychan.glb");
    registry.emplace<MaterialComponent>(entity).AddPass("pbr");
    
}

void Enemy::Update(float elapsedTime, entt::registry& registry, const SceneContext& context)
{
    auto* animator = registry.try_get<AnimatorComponent>(entity);
    if (animator)
    {
        animator->currentAnimation = "Anim_Idle_0";
        animator->loop = true;
        animator->playSpeed = 1.0f;
    }

}

void Enemy::RenderImGui()
{
}