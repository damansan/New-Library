#include "PlayerDodgeSystem.h"
#include "Game/ECS/Components/GameInputComponent.h"
#include "Game/ECS/Components/PlayerDodgeComponent.h"
#include "Game/ECS/Systems/PlayerMovementUtils.h"

#include "Engine/ECS/Components/AnimatorComponent.h"
#include "Engine/ECS/Components/StateComponent.h"
#include "Engine/ECS/Components/MovementComponent.h"
#include "Engine/ECS/Components/TransformComponent.h"
#include "Engine/ECS/Systems/StateGraphUtils.h"
#include <DirectXMath.h>

void PlayerDodgeSystem::Update(entt::registry& registry, float elapsedTime, const SceneContext& context)
{
	auto view = registry.view<GameInputComponent, PlayerDodgeComponent, AnimatorComponent, StateComponent, MovementComponent, TransformComponent>();

	view.each(
		[&](auto entity,
			GameInputComponent& input,
			PlayerDodgeComponent& dodge,
			AnimatorComponent& animator,
			StateComponent& state,
			MovementComponent& movement,
			TransformComponent& transform)
		{
			// AnimatorGraphPathが空ならDodge判定をfalseにする
			if (animator.animatorGraphPath.empty())
			{
				dodge.isDodging = false;
				return;
			}

			AnimatorGraphData& data = graphCache.GetOrLoad(animator.animatorGraphPath,
				[](AnimatorGraphData& data, const nlohmann::json& j) { data.FromJson(j); });

			// "Dodge"のIDをキャッシュするまで検索する
			if (dodgeStateId == -1)
			{
				dodgeStateId = StateGraphUtils::FindStateIdByName(data.parentGraph, "Dodge");
			}

			// 現在がDodgeかどうかを判定する
			bool isMyState = (state.currentStateId == dodgeStateId);

			// Dodgeでなければ、以降の処理は行わない
			if (!isMyState)
			{
				dodge.isDodging = false;
				dodge.bufferTimer = 0.0f;
				dodge.isNoneInput = false;
				dodge.isFrontInput = false;
				dodge.isBackInput = false;
				dodge.isLeftInput = false;
				dodge.isRightInput = false;
				movement.facingLocked = false;
				movement.xzControlLocked = false;

				dodge.dodgeSub = PlayerDodgeSubState::None;
				return;
			}

			// カメラの前後左右ベクトル取得
			DirectX::XMFLOAT3 front = context.camera->GetFront();
			DirectX::XMFLOAT3 right = context.camera->GetRight();

			// Dodgeに入った直後は、方向決定の猶予状態にする
			if (dodge.dodgeSub == PlayerDodgeSubState::None)
			{
				dodge.dodgeSub = PlayerDodgeSubState::Deciding;
				dodge.bufferTimer = 0.0f;
			}

			// 方向決定の猶予中
			if (dodge.dodgeSub == PlayerDodgeSubState::Deciding)
			{
				// 入力方向と倒し具合を算出
				auto dir = PlayerMovementUtils::StickToWorldVelocity(input.moveStick.x, input.moveStick.y, front, right, 1.0f);

				// 方向あり回避
				if (dir.inputLength > 0.0f)
				{
					// 入力方向とキャラクターの向きの差を求める
					float inputYaw = PlayerMovementUtils::CalcFacingYaw(dir.velocity.x, dir.velocity.z);
					float diff = inputYaw - transform.rotation.y;

					// 角度差を正規化
					while (diff > DirectX::XM_PI)  diff -= DirectX::XM_2PI;
					while (diff < -DirectX::XM_PI) diff += DirectX::XM_2PI;

					const float quarter = DirectX::XM_PIDIV4; // 45度

					if (diff > -quarter && diff <= quarter)
						dodge.dodgeSub = PlayerDodgeSubState::Front;
					else if (diff > quarter && diff <= quarter * 3.0f)
						dodge.dodgeSub = PlayerDodgeSubState::Right;
					else if (diff > -quarter * 3.0f && diff <= -quarter)
						dodge.dodgeSub = PlayerDodgeSubState::Left;
					else
						dodge.dodgeSub = PlayerDodgeSubState::Back;

					// 向きとXZ方向の制御権を取得
					movement.facingLocked = true;
					movement.xzControlLocked = true;
				}
				else
				{
					// 猶予時間の消化
					dodge.bufferTimer += elapsedTime;
					if (dodge.bufferTimer >= dodge.directionBufferTime)
					{
						// スティック入力が来なかったので、その場回避に確定する
						dodge.dodgeSub = PlayerDodgeSubState::InPlace;
						movement.facingLocked = true;
						movement.xzControlLocked = true;
					}
				}
			}

			// フラグ更新
			dodge.isNoneInput = (dodge.dodgeSub == PlayerDodgeSubState::InPlace);
			dodge.isFrontInput = (dodge.dodgeSub == PlayerDodgeSubState::Front);
			dodge.isBackInput = (dodge.dodgeSub == PlayerDodgeSubState::Back);
			dodge.isLeftInput = (dodge.dodgeSub == PlayerDodgeSubState::Left);
			dodge.isRightInput = (dodge.dodgeSub == PlayerDodgeSubState::Right);
			dodge.isDodging = true;
		});
}