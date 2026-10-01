#pragma once
#include "Engine/Core/SceneContext.h"
#include "Engine/Core/JsonFileCache.h"
#include "Engine/Data/AnimatorGraphData.h"

class PlayerJumpSystem
{
public:
	void Update(entt::registry& registry, float elapsedTime, const SceneContext& context);

private:
	// "Jump"というStateの名前をキャッシュする
	JsonFileCache<AnimatorGraphData> graphCache;
	int jumpStateId = -1;

};