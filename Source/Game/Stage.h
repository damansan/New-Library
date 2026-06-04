#pragma once
#include "Game/Object.h"

class Stage : public Object
{
public:
    Stage() = default;
    virtual ~Stage() = default;

    void Initialize(entt::registry& registry, const SceneContext& context) override;
    void RenderImGui() override;
};