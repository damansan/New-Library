#include "PlayerMoveSystem.h"

#include "Project/ECS/Components/GameInputComponent.h"
#include "Project/ECS/Components/PlayerMoveComponent.h"
#include "Project/ECS/Systems/PlayerMovementUtils.h"
#include "Engine/ECS/Components/MovementComponent.h"
#include "Engine/ECS/Components/TransformComponent.h"

void PlayerMoveSystem::Update(entt::registry& registry, float elapsedTime, const SceneContext& context)
{
	auto view = registry.view<GameInputComponent, PlayerMoveComponent, MovementComponent, TransformComponent>();

	view.each(
		[&](auto entity,
		GameInputComponent& input,
		PlayerMoveComponent& playerMove,
		MovementComponent& movement,
		TransformComponent& transform)
		{
			// 移動がロックされている間は、何もしない
			if (movement.movementLocked) return;

			// カメラの前後左右ベクトル取得
			DirectX::XMFLOAT3 front = context.camera->GetFront();
			DirectX::XMFLOAT3 right = context.camera->GetRight();

			// スティック入力からワールド方向・倒し具合を算出
			auto dir = PlayerMovementUtils::StickToWorldVelocity(input.moveX, input.moveZ, front, right, 1.0f);

			// XZ平面で正規化
			float frontLen = sqrtf(front.x * front.x + front.z * front.z);
			float rightLen = sqrtf(right.x * right.x + right.z * right.z);

			float frontX = (frontLen > 0.0f) ? front.x / frontLen : 0.0f;
			float frontZ = (frontLen > 0.0f) ? front.z / frontLen : 0.0f;
			float rightX = (rightLen > 0.0f) ? right.x / rightLen : 0.0f;
			float rightZ = (rightLen > 0.0f) ? right.z / rightLen : 0.0f;

			// 移動方向ベクトル
			float vecX = frontX * input.moveZ + rightX * input.moveX;
			float vecZ = frontZ * input.moveZ + rightZ * input.moveX;
			float vecLen = sqrtf(vecX * vecX + vecZ * vecZ); // スティックの倒し具合

			// 向きの制御権を持っているときのみ更新する
			if (!movement.facingLocked && dir.inputLength > 0.0f)
			{
				float targetYaw = PlayerMovementUtils::CalcFacingYaw(dir.velocity.x, dir.velocity.z);
				transform.rotation.y = PlayerMovementUtils::RotateTowards(transform.rotation.y, targetYaw, playerMove.turnSpeed, elapsedTime);
			}

			// 制御権を取り戻した直後の1フレームかどうか
			bool justRegainedControl = playerMove.wasControlLocked && !movement.xzControlLocked;

			// 移動の制御権を持っているときのみ更新する
			if (!movement.xzControlLocked)
			{
				if (dir.inputLength > 0.0f)
				{
					// 入力中は猶予タイマーをリセット
					playerMove.inputLossTimer = 0.0f;

					// 倒し具合に応じて、Walking/Joggingを判定する
					playerMove.state = (dir.inputLength > playerMove.jogThreshold)
						? PlayerMovementState::Jogging
						: PlayerMovementState::Walking;

					float speed = (playerMove.state == PlayerMovementState::Jogging) ? playerMove.jogSpeed : playerMove.walkSpeed;

					// 改めて実際の速度で計算し直す
					auto vel = PlayerMovementUtils::StickToWorldVelocity(input.moveX, input.moveZ, front, right, speed);
					movement.velocity.x = vel.velocity.x;
					movement.velocity.z = vel.velocity.z;
				}
				else if (justRegainedControl)
				{
					// 制御を取り戻した瞬間、入力が無いなら猶予を挟まず即座に確定させる
					playerMove.state = PlayerMovementState::None;
					playerMove.inputLossTimer = 0.0f;
					movement.velocity.x = 0.0f;
					movement.velocity.z = 0.0f;
				}
				else
				{
					// 入力がない
					playerMove.inputLossTimer += elapsedTime;
					if (playerMove.inputLossTimer >= playerMove.inputLossGraceTime)
					{
						// 猶予切れ。ここで初めて停止させる
						playerMove.state = PlayerMovementState::None;
						movement.velocity.x = 0.0f;
						movement.velocity.z = 0.0f;
					}
				}
			}

			playerMove.wasControlLocked = movement.xzControlLocked;

			// フラグ更新
			playerMove.isWalking = (playerMove.state == PlayerMovementState::Walking);
			playerMove.isJogging = (playerMove.state == PlayerMovementState::Jogging);
			playerMove.isMoving = (playerMove.isWalking || playerMove.isJogging);
		});
}