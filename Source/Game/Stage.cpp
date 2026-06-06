#include "Stage.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/MeshComponent.h"
#include "ECS/Components/MaterialComponent.h"

void Stage::Initialize(entt::registry& registry, const SceneContext& context)
{
    entity = registry.create();

    auto& t = registry.emplace<TransformComponent>(entity);
    t.position = { 0.0f, 0.0f, 0.0f };
    t.scale = { 1.0f, 1.0f, 1.0f };

    registry.emplace<MeshComponent>(entity, "Data/Model/Stage/ExampleStage/ExampleStage.gltf");
    registry.emplace<MaterialComponent>(entity).AddPass("pbr");
}

void Stage::RenderImGui()
{
}