#pragma once
#include "entt.hpp"
#include "Engine/ECS/Components/CategoryComponent.h"

#include <vector>

// w’è‚µ‚½‘®«‚ğ‚ÂÅ‰‚ÌEntity‚ğ—ñ‹“
inline entt::entity FindFirstByCategory(entt::registry& registry, EntityCategory category)
{
	auto view = registry.view<ComponentCategory>();
	for (auto entity : view)
	{
		if (view.get<ComponentCategory>(entity).category == category)
		{
			return entity;
		}
	}
	return entt::null;
}

// w’è‚µ‚½‘®«‚ğ‚Â‘S•”‚ÌEntity‚ğ—ñ‹“
inline std::vector<entt::entity> FindAllByCategory(entt::registry& registry, EntityCategory category)
{
	std::vector<entt::entity> result;

	auto view = registry.view<ComponentCategory>();
	for (auto entity : view)
	{
		if (view.get<ComponentCategory>(entity).category == category)
		{
			result.push_back(entity);
		}
	}
	return result;
}