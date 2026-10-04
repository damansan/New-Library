#include "PlayerJumpSystem.h"
#include "Game/ECS/Components/PlayerJumpComponent.h"

#include "Engine/ECS/Components/AnimatorComponent.h"
#include "Engine/ECS/Components/StateComponent.h"
#include "Engine/ECS/Components/MovementComponent.h"
#include "Engine/ECS/Systems/StateGraphUtils.h"


void PlayerJumpSystem::Update(entt::registry& registry, float elapsedTime, const SceneContext& context)
{
	auto view = registry.view<PlayerJumpComponent, AnimatorComponent, StateComponent, MovementComponent>();

	view.each(
		[&](PlayerJumpComponent& jump,
			AnimatorComponent& animator,
			StateComponent& state,
			MovementComponent& movement)
		{
			// AnimatorGraphPathが空ならJump判定をfalseにする
			if (animator.animatorGraphPath.empty())
			{
				jump.isJumping = false;
				jump.jumpSub = PlayerJumpSubState::None;
				return;
			}

			AnimatorGraphData& data = graphCache.GetOrLoad(animator.animatorGraphPath,
				[](AnimatorGraphData& data, const nlohmann::json& j) { data.FromJson(j); });

			// "Jump"のIDをキャッシュするまで検索する
			if (jumpStateId == -1)
			{
				jumpStateId = StateGraphUtils::FindStateIdByName(data.parentGraph, "Jump");
			}

			// 現在がJumpかどうかを判定する
			bool isMyState = (state.currentStateId == jumpStateId);

			// Jumpでなければ、以降の処理は行わない
			if (!isMyState)
			{
				jump.isJumping = false;
				jump.jumpSub = PlayerJumpSubState::None;
				return;
			}

			// Jumpに入った最初のフレーム
			if (jump.jumpSub == PlayerJumpSubState::None)
			{
				if (movement.isGrounded)
				{
					// 地上から入った：ジャンプ
					movement.velocity.y = jump.jumpPower;
					jump.jumpSub = PlayerJumpSubState::Start;
				}
				else
				{
					// 空中から入った：落下
					jump.jumpSub = PlayerJumpSubState::Loop;
				}
			}

			// 頂点を越えたらLoopへ
			if (jump.jumpSub == PlayerJumpSubState::Start && movement.velocity.y <= 0.0f)
			{
				jump.jumpSub = PlayerJumpSubState::Loop;
			}

			// フラグ更新
			jump.isJumping = true;
		});
}