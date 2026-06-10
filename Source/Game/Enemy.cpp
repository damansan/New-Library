#include "Enemy.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/MeshComponent.h"
#include "ECS/Components/MaterialComponent.h"

void Enemy::Initialize(entt::registry& registry, const SceneContext& context)
{
    entity = registry.create();

    auto& t = registry.emplace<TransformComponent>(entity);
    t.position = { 0.0f, 0.0f, 0.0f };
    t.scale = { 1.0f, 1.0f, 1.0f };

    registry.emplace<MeshComponent>(entity, "Data/Model/EnemyRobot/SK_SteamPunkBoss.gltf");
    registry.emplace<MaterialComponent>(entity).AddPass("pbr");
}

void Enemy::Update(float elapsedTime, entt::registry& registry, const SceneContext& context)
{
}

void Enemy::RenderImGui()
{
}