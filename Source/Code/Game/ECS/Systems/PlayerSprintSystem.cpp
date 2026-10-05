#include "PlayerSprintSystem.h"
#include "Game/ECS/Components/GameInputComponent.h"
#include "Game/ECS/Components/PlayerSprintComponent.h"
#include "Game/ECS/Systems/PlayerMovementUtils.h"

#include "Engine/ECS/Components/StateMachineComponent.h"
#include "Engine/ECS/Components/MovementComponent.h"
#include "Engine/ECS/Components/TransformComponent.h"

#include "Engine/ECS/Systems/StateMachineSystem.h"

#include <DirectXMath.h>

PlayerSprintSystem::PlayerSprintSystem()
{
	StateMachineSystem::RequireState("Sprint", "PlayerSprintSystem");
}

void PlayerSprintSystem::Update(entt::registry& registry, float elapsedTime, const SceneContext& context)
{
	auto view = registry.view<GameInputComponent, PlayerSprintComponent, StateMachineComponent, MovementComponent, TransformComponent>();

	view.each(
		[&](auto entity,
			GameInputComponent& input,
			PlayerSprintComponent& sprint,
			StateMachineComponent& stateMachine,
			MovementComponent& movement,
			TransformComponent& transform)
		{
			// Sprintでなければ、以降の処理は行わない
			if (!stateMachine.IsIn("Sprint"))
			{
				sprint.isSprinting = false;

				sprint.state = PlayerSprintSubState::None;
				return;
			}

			// 移動がロックされている間は、何もしない
			if (movement.movementLocked) return;

			// カメラの前後左右ベクトル取得
			DirectX::XMFLOAT3 front = context.camera->GetFront();
			DirectX::XMFLOAT3 right = context.camera->GetRight();

			// スティック入力からワールド方向を算出
			auto dir = PlayerMovementUtils::StickToWorldVelocity(input.moveStick.x, input.moveStick.y, front, right, 1.0f);

			// 向きの制御権があるときのみ更新
			if (!movement.facingLocked && dir.inputLength > 0.0f)
			{
				float targetYaw = PlayerMovementUtils::CalcFacingYaw(dir.velocity.x, dir.velocity.z);
				transform.rotation.y = PlayerMovementUtils::RotateTowards(transform.rotation.y, targetYaw, sprint.turnSpeed, elapsedTime);
			}

			// 目標速度に合わせた速度を計算
			auto vel = PlayerMovementUtils::ResolvePlanarVelocity(input.moveStick.x, input.moveStick.y, front, right, sprint.sprintSpeed, movement.velocity, elapsedTime);
			movement.velocity.x = vel.velocity.x;
			movement.velocity.z = vel.velocity.z;

			// フラグ更新
			sprint.isSprinting = true;
		});
}