#include "TransitionControllerSystem.h"
#include "Engine/ECS/Components/AnimatorComponent.h"
#include "Engine/ECS/Components/StateComponent.h"
#include "ConditionGraphEvaluator.h"
#include "StateGraphUtils.h"

void TransitionControllerSystem::Update(entt::registry& registry, float elapsedTime, const SceneContext& context)
{
	auto view = registry.view<AnimatorComponent>();

	for (auto entity : view)
	{
		auto& animator = view.get<AnimatorComponent>(entity);

		if (animator.animatorGraphPath.empty())
		{
			continue;
		}

		auto& state = registry.get_or_emplace<StateComponent>(entity);

		AnimatorGraphData& data = graphCache.GetOrLoad(animator.animatorGraphPath,
			[](AnimatorGraphData& data, const nlohmann::json& j) { data.FromJson(j); });

		// 未初期化の場合、Entryから出ているリンクの遷移先を初期状態とする
		if (state.currentStateId == -1)
		{
			int entryId = StateGraphUtils::FindStateIdByName(data.parentGraph, "Entry");
			if (entryId == -1) continue;

			int initialId = FindTransition(data.transitionGraph.links, entryId, registry, entity);
			if (initialId == -1) continue;

			state.currentStateId = initialId;
			continue; // 遷移した最初のフレームは、それ以上の判定を行わない
		}

		// 現在地から出ているリンクを順番にチェック
		int newStateId = FindTransition(data.transitionGraph.links, state.currentStateId, registry, entity);
		if (newStateId != -1)
		{
			OutputDebugStringA(("Transition: " + std::to_string(state.currentStateId) + " -> " + std::to_string(newStateId) + "\n").c_str());
			state.currentStateId = newStateId;
		}

	}
}

// 現在Stateから遷移可能なLinkを探す
int TransitionControllerSystem::FindTransition(const std::vector<TransitionLink>& links, int fromId, entt::registry& registry, entt::entity entity)
{
	for (const auto& link : links)
	{
		if (link.fromId != fromId) continue;
		if (!ConditionGraphEvaluator::Evaluate(link.condition, registry, entity)) continue;

		return link.toId; // 最初に条件が成立したものを採用する(複数該当時は先勝ち)
	}
	return -1;
}
