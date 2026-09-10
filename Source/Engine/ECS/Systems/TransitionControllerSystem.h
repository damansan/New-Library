#pragma once
#include "entt.hpp"
#include "Engine/Core/SceneContext.h"
#include "Engine/Core/JsonFileCache.h"
#include "Engine/Data/AnimatorGraphData.h"

class TransitionControllerSystem
{
public:
	void Update(entt::registry& registry, float elapsedTime, const SceneContext& context);

private:
	JsonFileCache<AnimatorGraphData> graphCache;

	// Œ»İState‚©‚ç‘JˆÚ‰Â”\‚ÈLink‚ğ’T‚·
	int FindTransition(const std::vector<TransitionLink>& links, int fromId, entt::registry& registry, entt::entity entity);
};