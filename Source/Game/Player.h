#pragma once
#include "Character.h"


class Player : public Character
{
public:
    Player() = default;
    virtual ~Player() = default;

    void Initialize(entt::registry& registry, const SceneContext& context) override;
    void Update(float elapsedTime, entt::registry& registry, const SceneContext& context) override;
    void RenderImGui() override;
};