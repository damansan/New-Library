#include "PlayerMoveSystem.h"

#include "ECS/Components/GameInputComponent.h"
#include "ECS/Components/PlayerMoveComponent.h"
#include "ECS/Components/MovementComponent.h"
#include "ECS/Components/TransformComponent.h"

void PlayerMoveSystem::Update(entt::registry& registry, float elapsedTime, const SceneContext& context)
{
	auto view = registry.view<GameInputComponent, PlayerMoveComponent, MovementComponent, TransformComponent>();

	view.each([&](auto entity,
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




			// 倒し具合に応じて、Idle/歩き/小走りを判定する
			playerMove.isJogging = (vecLen > playerMove.jogThreshold);
			playerMove.isWalking = (vecLen > 0.0f && !playerMove.isJogging);
			playerMove.isMoving = (playerMove.isWalking || playerMove.isJogging);
			float speed = playerMove.isJogging ? playerMove.jogSpeed : playerMove.walkSpeed;

			// 移動ベクトルを正規化して速度をかける
			float moveX = 0.0f;
			float moveZ = 0.0f;
			if (vecLen > 0.0f)
			{
				moveX = (vecX / vecLen) * speed;
				moveZ = (vecZ / vecLen) * speed;
			}

			// XZ方向だけをMovementComponentへ渡す
			movement.velocity.x = moveX;
			movement.velocity.z = moveZ;

			// キャラクターの向きを変える
			if (vecLen > 0.0f)
			{
				transform.rotation.y = atan2f(-moveX, -moveZ);
			}


		});
}