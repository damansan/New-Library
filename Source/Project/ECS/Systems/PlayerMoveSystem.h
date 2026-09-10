#pragma once
#include "entt.hpp"
#include "Engine/Core/SceneContext.h"
#include "Engine/Core/JsonFileCache.h"
#include "Engine/Data/AnimatorGraphData.h"

class PlayerMoveSystem
{
public:
	void Update(entt::registry& registry, float elapsedTime, const SceneContext& context);

private:
	// "Move"というStateの名前をキャッシュする
	JsonFileCache<AnimatorGraphData> graphCache;
	int moveStateId = -1;
};