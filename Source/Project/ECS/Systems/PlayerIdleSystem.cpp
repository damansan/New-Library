#include "PlayerIdleSystem.h"
#include "Project/ECS/Components/PlayerIdleComponent.h"

#include "Engine/ECS/Components/AnimatorComponent.h"
#include "Engine/ECS/Components/StateComponent.h"
#include "Engine/ECS/Systems/StateGraphUtils.h"

void PlayerIdleSystem::Update(entt::registry& registry, float elapsedTime, const SceneContext& context)
{
	auto view = registry.view<AnimatorComponent, StateComponent, PlayerIdleComponent>();

	view.each(
		[&](auto entity,
			AnimatorComponent& animator,
			StateComponent& state,
			PlayerIdleComponent& idle)
		{
			// AnimatorGraphPathが空ならIdle判定をfalseにする
			if (animator.animatorGraphPath.empty())
			{
				idle.isIdling = false;
				return;
			}

			AnimatorGraphData& data = graphCache.GetOrLoad(animator.animatorGraphPath,
				[](AnimatorGraphData& data, const nlohmann::json& j) { data.FromJson(j); });

			// "Idle"のIDをキャッシュするまで検索する
			if (idleStateId == -1)
			{
				idleStateId = StateGraphUtils::FindStateIdByName(data.parentGraph, "Idle");
			}

			// 現在がIdleかどうかを判定する
			bool isMyState = (state.currentStateId == idleStateId);

			// Idleでなければ、以降の処理は行わない
			if (!isMyState)
			{
				idle.isIdling = false;
				return;
			}

			// フラグ更新
			idle.isIdling = true;
		});
}