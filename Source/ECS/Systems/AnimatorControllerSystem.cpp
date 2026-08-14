#include "AnimatorControllerSystem.h"
#include "ECS/Components/AnimatorComponent.h"
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

		AnimatorGraphData& data = GetOrLoadAnimatorGraphData(animator.animatorGraphPath);

		if (animator.currentParentStateId == -1)
		{
			InitializeState(animator, data, registry, entity);
			continue;
		}


		// 親階層: 毎フレーム、子の状態に関わらず最優先でチェックする
		int newParentId = FindTransition(data.parentGraph.stateLinks, animator.currentParentStateId, registry, entity);
		if (newParentId != -1)
		{
			EnterParentState(animator, data, newParentId, registry, entity);
			continue;
		}

		// 子階層: 親の遷移が無かった場合のみチェックする
		ParentState* parent = FindParentStateById(data.parentGraph, animator.currentParentStateId);
		if (!parent) continue;

		int newChildId = FindTransition(parent->childGraph.childLinks, animator.currentChildStateId, registry, entity);
		if (newChildId == -1) continue;

		ChildState* newChild = FindChildStateById(parent->childGraph, newChildId);
		if (!newChild) continue;

		if (newChild->type == ChildStateType::End)
		{
			// 同じ子階層のStartへ戻ってやり直す
			EnterChildGraphFromStart(animator, *parent, registry, entity);
		}
		else
		{
			animator.currentChildStateId = newChildId;
			ApplyChildState(animator, *newChild);
		}
	}
}

// パスから、対応するAnimatorGraphDataを取得する
AnimatorGraphData& AnimatorControllerSystem::GetOrLoadAnimatorGraphData(const std::string& path)
{
	auto it = cache.find(path);
	if (it != cache.end())
	{
		return it->second;
	}

	// マップの中に直接デフォルト構築してから、その場でJsonを読み込む
	auto result = cache.try_emplace(path);
	AnimatorGraphData& data = result.first->second;

	std::ifstream ifs(path);
	if (ifs.is_open())
	{
		try
		{
			nlohmann::json j;
			ifs >> j;
			data.FromJson(j);
		}
		catch (const std::exception&) {}
	}

	return data;
}

// 遷移条件が成立している物を探す、複数ある場合は最初に見つかったものを通し、警告を出す
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
			// ここに警告出す
		}
	}
	return result;
}

// ParentStateGraphの中から、Entryのidを探す
int AnimatorControllerSystem::FindEntryParentState(const ParentStateGraph& parentGraph)
{
	for (const auto& s : parentGraph.states)
		if (s.type == ParentStateType::Entry) return s.id;
	return -1;
}
// ChildStateGraphの中から、Start/Endを探す
int AnimatorControllerSystem::FindChildStateByType(const ChildStateGraph& childGraph, ChildStateType type)
{
	for (const auto& c : childGraph.children)
		if (c.type == type) return c.id;
	return -1;
}

// idから実体を探す
ParentState* AnimatorControllerSystem::FindParentStateById(ParentStateGraph& parentGraph, int id)
{
	for (auto& s : parentGraph.states)
		if (s.id == id) return &s;
	return nullptr;
}
// idから実体を探す
ChildState* AnimatorControllerSystem::FindChildStateById(ChildStateGraph& childGraph, int id)
{
	for (auto& c : childGraph.children)
		if (c.id == id) return &c;
	return nullptr;
}

// ParentStateの子階層のStartから最初のChildStateまで辿って適用する
void AnimatorControllerSystem::EnterParentState(AnimatorComponent& animator, AnimatorGraphData& data, int parentId, entt::registry& registry, entt::entity entity)
{
	ParentState* parent = FindParentStateById(data.parentGraph, parentId);
	if (!parent) return;

	animator.currentParentStateId = parentId;
	EnterChildGraphFromStart(animator, *parent, registry, entity);
}
// Parentの子階層のStartから最初のChildStateまで辿って適用する
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
// 初回の初期状態を決める
void AnimatorControllerSystem::InitializeState(AnimatorComponent& animator, AnimatorGraphData& data, entt::registry& registry, entt::entity entity)
{
	int entryId = FindEntryParentState(data.parentGraph);
	if (entryId == -1) return;

	int parentId = FindTransition(data.parentGraph.stateLinks, entryId, registry, entity);
	if (parentId == -1) return;

	EnterParentState(animator, data, parentId, registry, entity);
}
// ChildStateの内容をAnimatorComponentへ反映する
void AnimatorControllerSystem::ApplyChildState(AnimatorComponent& animator, const ChildState& child)
{
	animator.currentAnimation = child.clipName;
	animator.animationTime = 0.0f;
	animator.isPlaying = true;
	animator.isEnd = false;
	animator.useRootMotion = child.useRootMotion;
	animator.loop = child.loop;
}
