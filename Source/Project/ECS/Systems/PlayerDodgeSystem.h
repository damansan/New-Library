#pragma once
#include "entt.hpp"
#include "Engine/Core/SceneContext.h"
#include "Engine/Core/JsonFileCache.h"
#include "Engine/Data/AnimatorGraphData.h"

class PlayerDodgeSystem
{
public:
	void Update(entt::registry& registry, float elapsedTime, const SceneContext& context);

private:
	// "Dodge"というStateの名前をキャッシュする
	JsonFileCache<AnimatorGraphData> graphCache;
	int dodgeStateId = -1; 
};