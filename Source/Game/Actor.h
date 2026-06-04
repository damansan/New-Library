#pragma once
#include "entt.hpp"
#include "Core/SceneContext.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/MeshComponent.h"
#include "ECS/Components/MaterialComponent.h"

class Actor
{
public:
    Actor() = default;
    virtual ~Actor() = default;

    // 初期化・終了
    virtual void Initialize(entt::registry& registry, const SceneContext& context) = 0;
    virtual void Finalize(entt::registry& registry) {}

    // 更新・ImGui
    virtual void Update(float elapsedTime, entt::registry& registry, const SceneContext& context) {}
    virtual void RenderImGui() {}

    // entityの取得
    entt::entity GetEntity() const { return entity; }

protected:
    entt::entity entity{ entt::null };
};