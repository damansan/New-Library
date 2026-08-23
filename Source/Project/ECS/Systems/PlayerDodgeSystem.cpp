#include "PlayerDodgeSystem.h"

#include "Project/ECS/Components/GameInputComponent.h"
#include "Project/ECS/Components/PlayerDodgeComponent.h"
#include "Project/ECS/Systems/PlayerMovementUtils.h"
#include "Project/ECS/Components/PlayerSprintComponent.h"

#include "Engine/ECS/Components/MovementComponent.h"
#include "Engine/ECS/Components/AnimatorComponent.h"
#include "Engine/ECS/Components/SequencerComponent.h"
#include "Engine/ECS/Components/TransformComponent.h"
#include <DirectXMath.h>

void PlayerDodgeSystem::Update(entt::registry& registry, float elapsedTime, const SceneContext& context)
{
	auto view = registry.view<GameInputComponent, PlayerDodgeComponent, MovementComponent, AnimatorComponent, SequencerComponent, TransformComponent>();

	view.each([&](auto entity,
		GameInputComponent& input,
		PlayerDodgeComponent& dodge,
		MovementComponent& movement,
		AnimatorComponent& animator,
		SequencerComponent& sequencer,
		TransformComponent& transform)
		{
			// 既に押している間は 離した判定ライン を下回るまで押している扱いにする
			bool triggerHeld = dodge.wasTriggerHeld
				? (input.dodge > dodge.triggerReleaseThreshold)
				: (input.dodge >= dodge.triggerThreshold);
			bool triggerPressed = (triggerHeld && !dodge.wasTriggerHeld);

			// R2入力が来たら、Deciding状態に遷移する
			if (dodge.state == PlayerDodgeState::None && triggerPressed)
			{
				dodge.state = PlayerDodgeState::Deciding;
				dodge.bufferTimer = 0.0f;
			}

			// スティック入力が来たら、４方向のいずれかに遷移する
			// それまでに入力が来なければ、単回避に遷移する
			if (dodge.state == PlayerDodgeState::Deciding)
			{
				DirectX::XMFLOAT3 front = context.camera->GetFront();
				DirectX::XMFLOAT3 right = context.camera->GetRight();
				
				// 入力方向と倒し具合を算出
				auto dir = PlayerMovementUtils::StickToWorldVelocity(input.moveX, input.moveZ, front, right, 1.0f);

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
						dodge.state = PlayerDodgeState::FrontDodge;
					else if (diff > quarter && diff <= quarter * 3.0f)
						dodge.state = PlayerDodgeState::RightDodge;
					else if (diff > -quarter * 3.0f && diff <= -quarter)
						dodge.state = PlayerDodgeState::LeftDodge;
					else
						dodge.state = PlayerDodgeState::BackDodge;

					// 向きの制御権を取得
					movement.facingLocked = true;
					// XZ方向の制御権を取得
					movement.xzControlLocked = true;
				}
				else
				{
					// 猶予時間の消化
					dodge.bufferTimer += elapsedTime;
					if (dodge.bufferTimer >= dodge.directionBufferTime)
					{
						// スティック入力が来なかったので、単回避に遷移する
						dodge.state = PlayerDodgeState::Dodge;
						movement.facingLocked = true;
						movement.xzControlLocked = true;
					}
				}
			}
			// ４方向回避の終了判定
			else if (dodge.state != PlayerDodgeState::None)
			{
				// アニメーション終了範囲かつR2が離されたら、回避終了
				bool exitRequested = sequencer.animExit && !triggerHeld;

				if (animator.isEnd || exitRequested)
				{
					dodge.state = PlayerDodgeState::None;
					movement.xzControlLocked = false;
					movement.facingLocked = false;
				}
			}

			// 前フレームのR2押下状態を保存
			dodge.wasTriggerHeld = triggerHeld;

			// フラグ更新
			bool activeDodge = (dodge.state != PlayerDodgeState::None && dodge.state != PlayerDodgeState::Deciding);
			dodge.isDodging = activeDodge;
			dodge.isNoneInput = (dodge.state == PlayerDodgeState::Dodge);
			dodge.isFrontInput = (dodge.state == PlayerDodgeState::FrontDodge);
			dodge.isBackInput = (dodge.state == PlayerDodgeState::BackDodge);
			dodge.isLeftInput = (dodge.state == PlayerDodgeState::LeftDodge);
			dodge.isRightInput = (dodge.state == PlayerDodgeState::RightDodge);
		});
}