#include "PlayerMoveSystem.h"
#include "Game/ECS/Components/GameInputComponent.h"
#include "Game/ECS/Components/PlayerMoveComponent.h"
#include "Game/ECS/Systems/PlayerMovementUtils.h"

#include "Engine/ECS/Components/StateMachineComponent.h"
#include "Engine/ECS/Components/MovementComponent.h"
#include "Engine/ECS/Components/TransformComponent.h"
#include "Engine/ECS/Systems/StateMachineSystem.h"

PlayerMoveSystem::PlayerMoveSystem()
{
	StateMachineSystem::RequireState<PlayerMoveComponent>("Move", "PlayerMoveSystem");
}

void PlayerMoveSystem::Update(entt::registry& registry, float elapsedTime, const SceneContext& context)
{
	auto view = registry.view<GameInputComponent, PlayerMoveComponent, StateMachineComponent, MovementComponent, TransformComponent>();

	view.each(
		[&](auto entity,
			GameInputComponent& input,
			PlayerMoveComponent& playerMove,
			StateMachineComponent& stateMachine,
			MovementComponent& movement,
			TransformComponent& transform)
		{
			// Moveでなければ、以降の処理は行わない
			if (!stateMachine.IsIn("Move"))
			{
				playerMove.isMoving = false;
				playerMove.isWalking = false;
				playerMove.isJogging = false;

				playerMove.moveSub = PlayerMoveSubState::None;
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
				transform.rotation.y = PlayerMovementUtils::RotateTowards(transform.rotation.y, targetYaw, playerMove.turnSpeed, elapsedTime);
			}

			// 移動の制御権があるときのみ更新
			if (!movement.xzControlLocked)
			{
				if (dir.inputLength > 0.0f)
				{
					// 入力ロス猶予タイマーをリセット
					playerMove.inputLossTimer = 0.0f;

					// 入力量に応じて、Walking/Joggingを判定
					playerMove.moveSub = (dir.inputLength > playerMove.jogThreshold)
						? PlayerMoveSubState::Jogging
						: PlayerMoveSubState::Walking;

					float speed = (playerMove.moveSub == PlayerMoveSubState::Jogging) ? playerMove.jogSpeed : playerMove.walkSpeed;

					// 目標速度に合わせた速度を計算
					auto vel = PlayerMovementUtils::ResolvePlanarVelocity(input.moveStick.x, input.moveStick.y, front, right, speed, movement.velocity, elapsedTime);
					movement.velocity.x = vel.velocity.x;
					movement.velocity.z = vel.velocity.z;
				}
				else
				{
					// 入力なし猶予中
					playerMove.inputLossTimer += elapsedTime;
					if (playerMove.inputLossTimer >= playerMove.inputLossGraceTime)
					{
						// 入力ロス確定。動きを止める
						playerMove.moveSub = PlayerMoveSubState::None;
						movement.velocity.x = 0.0f;
						movement.velocity.z = 0.0f;
					}
				}
			}

			// フラグ更新
			playerMove.isWalking = (playerMove.moveSub == PlayerMoveSubState::Walking);
			playerMove.isJogging = (playerMove.moveSub == PlayerMoveSubState::Jogging);
			playerMove.isMoving = true;
		});
}