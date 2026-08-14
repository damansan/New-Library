#pragma once
#include "entt.hpp"
#include "Data/AnimatorGraphData.h"
#include "ECS/Components/AnimatorComponent.h"
#include <unordered_map>
#include <string>

class AnimatorControllerSystem
{
public:
	void Update(entt::registry& registry);

private:
	// ファイルパスごとに、読み込み済みのAnimatorGraphDataをキャッシュする
	std::unordered_map<std::string, AnimatorGraphData> cache;
	// パスから、対応するAnimatorGraphDataを取得する
	AnimatorGraphData& GetOrLoadAnimatorGraphData(const std::string& path);

	// 遷移条件が成立している物を探す、複数ある場合は最初に見つかったものを通し、警告を出す
	static int FindTransition(const std::vector<AnimLink>& links, int fromId, entt::registry& registry, entt::entity entity);

	// ParentStateGraphの中から、Entryのidを探す
	static int FindEntryParentState(const ParentStateGraph& parentGraph);
	// ChildStateGraphの中から、Start/Endを探す
	static int FindChildStateByType(const ChildStateGraph& childGraph, ChildStateType type);

	// idから実体を探す
	static ParentState* FindParentStateById(ParentStateGraph& parentGraph, int id);
	static ChildState* FindChildStateById(ChildStateGraph& childGraph, int id);


	// ParentStateの子階層のStartから最初のChildStateまで辿って適用する
	void EnterParentState(AnimatorComponent& animator, AnimatorGraphData& data, int parentId, entt::registry& registry, entt::entity entity);
	// Parentの子階層のStartから最初のChildStateまで辿って適用する
	void EnterChildGraphFromStart(AnimatorComponent& animator, ParentState& parent, entt::registry& registry, entt::entity entity);
	// 初回の初期状態を決める
	void InitializeState(AnimatorComponent& animator, AnimatorGraphData& data, entt::registry& registry, entt::entity entity);
	// ChildStateの内容をAnimatorComponentへ反映する
	void ApplyChildState(AnimatorComponent& animator, const ChildState& child);
};