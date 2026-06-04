#pragma once
#include "Character.h"

class Enemy : public Character
{
public:
    Enemy() = default;
    virtual ~Enemy() = default;

    void Initialize(entt::registry& registry, const SceneContext& context) override;
    void Update(float elapsedTime, entt::registry& registry, const SceneContext& context) override;
    void RenderImGui() override;
};