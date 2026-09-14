#include "PlayerMoveSystem.h"
#include "Project/ECS/Components/GameInputComponent.h"
#include "Project/ECS/Components/PlayerMoveComponent.h"
#include "Project/ECS/Systems/PlayerMovementUtils.h"

#include "Engine/ECS/Components/AnimatorComponent.h"
#include "Engine/ECS/Components/StateComponent.h"
#include "Engine/ECS/Components/MovementComponent.h"
#include "Engine/ECS/Components/TransformComponent.h"
#include "Engine/ECS/Systems/StateGraphUtils.h"

void PlayerMoveSystem::Update(entt::registry& registry, float elapsedTime, const SceneContext& context)
{
	auto view = registry.view<GameInputComponent, PlayerMoveComponent, AnimatorComponent, StateComponent, MovementComponent, TransformComponent>();

	view.each(
		[&](auto entity,
			GameInputComponent& input,
			PlayerMoveComponent& playerMove,
			AnimatorComponent& animator,
			StateComponent& state,
			MovementComponent& movement,
			TransformComponent& transform)
		{
			// AnimatorGraphPathが空ならMove判定をfalseにする
			if (animator.animatorGraphPath.empty())
			{
				playerMove.isMoving = false;
				playerMove.isWalking = false;
				playerMove.isJogging = false;
				return;
			}

			AnimatorGraphData& data = graphCache.GetOrLoad(animator.animatorGraphPath,
				[](AnimatorGraphData& data, const nlohmann::json& j) { data.FromJson(j); });

			// "Move"のIDをキャッシュするまで検索する
			if (moveStateId == -1)
			{
				moveStateId = StateGraphUtils::FindStateIdByName(data.parentGraph, "Move");
			}

			// 現在がMoveかどうかを判定する
			bool isMyState = (state.currentStateId == moveStateId);

			// Moveでなければ、以降の処理は行わない
			if (!isMyState)
			{
				playerMove.isMoving = false;
				playerMove.isWalking = false;
				playerMove.isJogging = false;

				playerMove.moveSub = PlayerMoveSubState::None;
				movement.velocity.x = 0.0f;
				movement.velocity.z = 0.0f;
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