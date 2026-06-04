#pragma once
#include "Actor.h"

class Character : public Actor
{
public:
    Character() = default;
    virtual ~Character() = default;

    virtual void Initialize(
        entt::registry& registry, 
        const SceneContext& context) = 0;
    
    virtual void Update(
        float elapsedTime, 
        entt::registry& registry, 
        const SceneContext& context) = 0;

protected:
};