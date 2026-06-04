#pragma once
#include "Actor.h"

class Object : public Actor
{
public:
    Object() = default;
    virtual ~Object() = default;

    virtual void Initialize(entt::registry& registry, const SceneContext& context) = 0;
    virtual void Update(float elapsedTime, entt::registry& registry, const SceneContext& context) {}
};