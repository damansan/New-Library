#include "PlayerMoveSystem.h"
#include "Game/ECS/Components/PlayerIntentComponent.h"
#include "Game/ECS/Components/PlayerMoveComponent.h"
#include "Game/ECS/Components/PlayerLocomotionComponent.h"

#include "Engine/ECS/Components/StateMachineComponent.h"
#include "Engine/ECS/Systems/StateMachineSystem.h"

PlayerMoveSystem::PlayerMoveSystem()
{
	StateMachineSystem::RequireState<PlayerMoveComponent>("Move", "PlayerMoveSystem");
}

void PlayerMoveSystem::Update(entt::registry& registry, float elapsedTime, const SceneContext& context)
{
	(void)elapsedTime;
	(void)context;

	auto view = registry.view<PlayerIntentComponent, PlayerMoveComponent, StateMachineComponent, PlayerLocomotionComponent>();
	view.each([&](const PlayerIntentComponent& intent, const PlayerMoveComponent& move,
		const StateMachineComponent& stateMachine, PlayerLocomotionComponent& loco)
		{
			if (!stateMachine.IsIn("Move")) return;

			// à”ê}ÇÅALocomotion Ç÷ÇÃéwé¶Ç…ïœÇ¶ÇÈ
			LocomotionCommand& cmd = loco.command;
			cmd.mode = LocomotionMode::Ground;
			cmd.moveDir = intent.moveDir;
			cmd.targetSpeed = intent.wantsRun ? move.runSpeed : move.walkSpeed;
			cmd.accelHalfLife = move.accelHalfLife;
			cmd.decelHalfLife = move.decelHalfLife;
			cmd.turnSpeed = move.turnSpeed;
		});
}