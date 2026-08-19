#pragma once
#include "entt.hpp"
#include "Engine/Data/AnimatorGraphData.h"
#include "Engine/ECS/Components/AnimatorComponent.h"
#include "Engine/Core/JsonFileCache.h"

#include <unordered_map>
#include <string>

class AnimatorControllerSystem
{
public:
	void Update(entt::registry& registry);

private:
	JsonFileCache<AnimatorGraphData> graphCache;

	// ‘JˆÚğŒ‚ª¬—§‚µ‚Ä‚¢‚é•¨‚ğ’T‚·A•¡”‚ ‚éê‡‚ÍÅ‰‚ÉŒ©‚Â‚©‚Á‚½‚à‚Ì‚ğ’Ê‚µAŒx‚ğo‚·
	static int FindTransition(const std::vector<AnimLink>& links, int fromId, entt::registry& registry, entt::entity entity);

	// ParentStateGraph‚Ì’†‚©‚çAEntry‚Ìid‚ğ’T‚·
	static int FindEntryParentState(const ParentStateGraph& parentGraph);
	// ChildStateGraph‚Ì’†‚©‚çAStart/End‚ğ’T‚·
	static int FindChildStateByType(const ChildStateGraph& childGraph, ChildStateType type);

	// id‚©‚çÀ‘Ì‚ğ’T‚·
	static ParentState* FindParentStateById(ParentStateGraph& parentGraph, int id);
	static ChildState* FindChildStateById(ChildStateGraph& childGraph, int id);


	// ParentState‚ÌqŠK‘w‚ÌStart‚©‚çÅ‰‚ÌChildState‚Ü‚Å’H‚Á‚Ä“K—p‚·‚é
	void EnterParentState(AnimatorComponent& animator, AnimatorGraphData& data, int parentId, entt::registry& registry, entt::entity entity);
	// Parent‚ÌqŠK‘w‚ÌStart‚©‚çÅ‰‚ÌChildState‚Ü‚Å’H‚Á‚Ä“K—p‚·‚é
	void EnterChildGraphFromStart(AnimatorComponent& animator, ParentState& parent, entt::registry& registry, entt::entity entity);
	// ‰‰ñ‚Ì‰Šúó‘Ô‚ğŒˆ‚ß‚é
	void InitializeState(AnimatorComponent& animator, AnimatorGraphData& data, entt::registry& registry, entt::entity entity);
	// ChildState‚Ì“à—e‚ğAnimatorComponent‚Ö”½‰f‚·‚é
	void ApplyChildState(AnimatorComponent& animator, const ChildState& child);
};