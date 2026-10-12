#include "PlayerJumpSystem.h"
#include "Game/ECS/Components/PlayerJumpComponent.h"

#include "Engine/ECS/Components/StateMachineComponent.h"
#include "Engine/ECS/Components/MovementComponent.h"

#include "Engine/ECS/Systems/StateMachineSystem.h"

PlayerJumpSystem::PlayerJumpSystem()
{
	StateMachineSystem::RequireState<PlayerJumpComponent>("Jump", "PlayerJumpSystem");
}

void PlayerJumpSystem::Update(entt::registry& registry, float elapsedTime, const SceneContext& context)
{
	auto view = registry.view<PlayerJumpComponent, StateMachineComponent, MovementComponent>();

	view.each(
		[&](PlayerJumpComponent& jump,
			StateMachineComponent& stateMachine,
			MovementComponent& movement)
		{
			// Jumpでなければ、以降の処理は行わない
			if (!stateMachine.IsIn("Jump"))
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