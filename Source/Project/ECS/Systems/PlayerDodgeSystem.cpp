#include "PlayerDodgeSystem.h"
#include "Project/ECS/Components/GameInputComponent.h"
#include "Project/ECS/Components/PlayerDodgeComponent.h"
#include "Project/ECS/Systems/PlayerMovementUtils.h"

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
			if (animator.animatorGraphPath.empty())
			{
				dodge.isDodging = false;
				return;
			}

			AnimatorGraphData& data = graphCache.GetOrLoad(animator.animatorGraphPath,
				[](AnimatorGraphData& data, const nlohmann::json& j) { data.FromJson(j); });

			// "Dodge"のIDをまだ持っていなければ、ここで検索してキャッシュする
			if (dodgeStateId == -1)
			{
				dodgeStateId = StateGraphUtils::FindStateIdByName(data.parentGraph, "Dodge");
			}

			// 毎フレーム必ず同期する(ガード句の前に置くのがポイント)
			bool isMyState = (state.currentStateId == dodgeStateId);
			dodge.isDodging = isMyState;

			if (!isMyState)
			{
				// Dodgeの担当を離れる時は、サブ状態と制御権を必ず手放す
				dodge.dodgeSub = PlayerDodgeSubState::None;
				dodge.bufferTimer = 0.0f;
				dodge.isNoneInput = false;
				dodge.isFrontInput = false;
				dodge.isBackInput = false;
				dodge.isLeftInput = false;
				dodge.isRightInput = false;
				movement.facingLocked = false;
				movement.xzControlLocked = false;
				return;
			}

			DirectX::XMFLOAT3 front = context.camera->GetFront();
			DirectX::XMFLOAT3 right = context.camera->GetRight();

			// Dodgeに入った直後、まだ方向が決まっていない場合
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

					// 角度差を-180～180の範囲に正規化
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
		});
}