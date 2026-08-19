#include "AnimatorControllerSystem.h"
#include "Engine/ECS/Components/AnimatorComponent.h"
#include "ConditionGraphEvaluator.h"
#include <fstream>

void AnimatorControllerSystem::Update(entt::registry& registry)
{
	auto view = registry.view<AnimatorComponent>();

	for (auto entity : view)
	{
		auto& animator = view.get<AnimatorComponent>(entity);

		if (animator.animatorGraphPath.empty())
		{
			continue;
		}

		AnimatorGraphData& data = graphCache.GetOrLoad(animator.animatorGraphPath,
			[](AnimatorGraphData& data, const nlohmann::json& j) { data.FromJson(j); });

		if (animator.currentParentStateId == -1)
		{
			InitializeState(animator, data, registry, entity);
			continue;
		}


		// eŠK‘w: –ˆƒtƒŒ[ƒ€Aq‚Ìó‘Ô‚ÉŠÖ‚í‚ç‚¸Å—Dæ‚Åƒ`ƒFƒbƒN‚·‚é
		int newParentId = FindTransition(data.parentGraph.stateLinks, animator.currentParentStateId, registry, entity);
		if (newParentId != -1)
		{
			EnterParentState(animator, data, newParentId, registry, entity);
			continue;
		}

		// qŠK‘w: e‚Ì‘JˆÚ‚ª–³‚©‚Á‚½ê‡‚Ì‚İƒ`ƒFƒbƒN‚·‚é
		ParentState* parent = FindParentStateById(data.parentGraph, animator.currentParentStateId);
		if (!parent)
		{
			animator.currentParentStateId = -1;
			continue;
		}

		int newChildId = FindTransition(parent->childGraph.childLinks, animator.currentChildStateId, registry, entity);
		if (newChildId == -1) continue;

		ChildState* newChild = FindChildStateById(parent->childGraph, newChildId);
		if (!newChild) continue;

		if (newChild->type == ChildStateType::End)
		{
			// “¯‚¶qŠK‘w‚ÌStart‚Ö–ß‚Á‚Ä‚â‚è’¼‚·
			EnterChildGraphFromStart(animator, *parent, registry, entity);
		}
		else
		{
			animator.currentChildStateId = newChildId;
			ApplyChildState(animator, *newChild);
		}
	}
}

// ‘JˆÚğŒ‚ª¬—§‚µ‚Ä‚¢‚é•¨‚ğ’T‚·A•¡”‚ ‚éê‡‚ÍÅ‰‚ÉŒ©‚Â‚©‚Á‚½‚à‚Ì‚ğ’Ê‚µAŒx‚ğo‚·
int AnimatorControllerSystem::FindTransition(const std::vector<AnimLink>& links, int fromId, entt::registry& registry, entt::entity entity)
{
	int result = -1;
	for (const auto& link : links)
	{
		if (link.fromId != fromId) continue;
		if (!ConditionGraphEvaluator::Evaluate(link.condition, registry, entity)) continue;

		if (result == -1)
		{
			result = link.toId;
		}
		else
		{
			// ‚±‚±‚ÉŒxo‚·
		}
	}
	return result;
}

// ParentStateGraph‚Ì’†‚©‚çAEntry‚Ìid‚ğ’T‚·
int AnimatorControllerSystem::FindEntryParentState(const ParentStateGraph& parentGraph)
{
	for (const auto& s : parentGraph.states)
		if (s.type == ParentStateType::Entry) return s.id;
	return -1;
}
// ChildStateGraph‚Ì’†‚©‚çAStart/End‚ğ’T‚·
int AnimatorControllerSystem::FindChildStateByType(const ChildStateGraph& childGraph, ChildStateType type)
{
	for (const auto& c : childGraph.children)
		if (c.type == type) return c.id;
	return -1;
}

// id‚©‚çÀ‘Ì‚ğ’T‚·
ParentState* AnimatorControllerSystem::FindParentStateById(ParentStateGraph& parentGraph, int id)
{
	for (auto& s : parentGraph.states)
		if (s.id == id) return &s;
	return nullptr;
}
// id‚©‚çÀ‘Ì‚ğ’T‚·
ChildState* AnimatorControllerSystem::FindChildStateById(ChildStateGraph& childGraph, int id)
{
	for (auto& c : childGraph.children)
		if (c.id == id) return &c;
	return nullptr;
}

// ParentState‚ÌqŠK‘w‚ÌStart‚©‚çÅ‰‚ÌChildState‚Ü‚Å’H‚Á‚Ä“K—p‚·‚é
void AnimatorControllerSystem::EnterParentState(AnimatorComponent& animator, AnimatorGraphData& data, int parentId, entt::registry& registry, entt::entity entity)
{
	ParentState* parent = FindParentStateById(data.parentGraph, parentId);
	if (!parent) return;

	animator.currentParentStateId = parentId;
	EnterChildGraphFromStart(animator, *parent, registry, entity);
}
// Parent‚ÌqŠK‘w‚ÌStart‚©‚çÅ‰‚ÌChildState‚Ü‚Å’H‚Á‚Ä“K—p‚·‚é
void AnimatorControllerSystem::EnterChildGraphFromStart(AnimatorComponent& animator, ParentState& parent, entt::registry& registry, entt::entity entity)
{
	int startId = FindChildStateByType(parent.childGraph, ChildStateType::Start);
	if (startId == -1) return;

	int childId = FindTransition(parent.childGraph.childLinks, startId, registry, entity);
	if (childId == -1) return;

	ChildState* child = FindChildStateById(parent.childGraph, childId);
	if (!child) return;

	animator.currentChildStateId = childId;
	ApplyChildState(animator, *child);
}
// ‰‰ñ‚Ì‰Šúó‘Ô‚ğŒˆ‚ß‚é
void AnimatorControllerSystem::InitializeState(AnimatorComponent& animator, AnimatorGraphData& data, entt::registry& registry, entt::entity entity)
{
	int entryId = FindEntryParentState(data.parentGraph);
	if (entryId == -1) return;

	int parentId = FindTransition(data.parentGraph.stateLinks, entryId, registry, entity);
	if (parentId == -1) return;

	EnterParentState(animator, data, parentId, registry, entity);
}
// ChildState‚Ì“à—e‚ğAnimatorComponent‚Ö”½‰f‚·‚é
void AnimatorControllerSystem::ApplyChildState(AnimatorComponent& animator, const ChildState& child)
{
	animator.currentAnimation = child.clipName;
	animator.animationTime = 0.0f;
	animator.isPlaying = true;
	animator.isEnd = false;
	animator.useRootMotion = child.useRootMotion;
	animator.loop = child.loop;
}
