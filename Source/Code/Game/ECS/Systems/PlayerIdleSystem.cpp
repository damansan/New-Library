#include "PlayerIdleSystem.h"
#include "Game/ECS/Components/PlayerIdleComponent.h"

#include "Engine/ECS/Components/StateMachineComponent.h"
#include "Engine/ECS/Components/MovementComponent.h"

#include "Engine/ECS/Systems/StateMachineSystem.h"

PlayerIdleSystem::PlayerIdleSystem()
{
	StateMachineSystem::RequireState<PlayerIdleComponent>("Idle", "PlayerIdleSystem");
}

void PlayerIdleSystem::Update(entt::registry& registry, float elapsedTime, const SceneContext& context)
{
	auto view = registry.view<StateMachineComponent, PlayerIdleComponent, MovementComponent>();

	view.each(
		[&](auto entity,
			StateMachineComponent& stateMachine,
			PlayerIdleComponent& idle,
			MovementComponent& movement) 
		{
			// Idleでなければ、以降の処理は行わない
			if (!stateMachine.IsIn("Idle"))
			{
				idle.isIdling = false;
				return;
			}

			// 水平方向の移動を止める
			movement.velocity.x = 0.0f;
			movement.velocity.z = 0.0f;

			// フラグ更新
			idle.isIdling = true;
		});
}