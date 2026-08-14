#pragma once
#include "entt.hpp"
#include "Core/SceneContext.h"

class PlayerMoveSystem
{
public:
	void Update(entt::registry& registry, float elapsedTime, const SceneContext& context);
};