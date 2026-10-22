#include "PlayerDodgeSystem.h"
#include <cmath>
#include <algorithm>

#include "Game/ECS/Components/PlayerIntentComponent.h"
#include "Game/ECS/Components/PlayerDodgeComponent.h"
#include "Game/ECS/Components/PlayerLocomotionComponent.h"
#include "Game/ECS/Systems/PlayerMovementUtils.h"

#include "Engine/ECS/Components/StateMachineComponent.h"
#include "Engine/ECS/Components/TransformComponent.h"
#include "Engine/ECS/Systems/StateMachineSystem.h"

namespace
{
	// 体の向きから見た角度差から、4方向のどれかを決める
	PlayerDodgeDirection ClassifyDirection(float diff)
	{
		const float quarter = DirectX::XM_PIDIV4; // 45度

		if (diff > -quarter && diff <= quarter)         return PlayerDodgeDirection::Front;
		if (diff > quarter && diff <= quarter * 3.0f)   return PlayerDodgeDirection::Right;
		if (diff > -quarter * 3.0f && diff <= -quarter) return PlayerDodgeDirection::Left;
		return PlayerDodgeDirection::Back;
	}
	// 体から見た回避方向から、Blend のペアと B の割合を決める
	void UpdateBlendPair(PlayerDodgeComponent& dodge)
	{
		const float quarter = DirectX::XM_PIDIV2; // 90度

		// 前を 0、右回りに +(-π〜π)
		float angle = atan2f(dodge.localX, dodge.localZ);

		dodge.isFrontRight = false;
		dodge.isRightBack = false;
		dodge.isBackLeft = false;
		dodge.isLeftFront = false;

		float rate = 0.0f;
		if (angle >= 0.0f && angle < quarter)
		{
			dodge.isFrontRight = true;                 // 前(0°) → 右(90°)
			rate = angle / quarter;
		}
		else if (angle >= quarter)
		{
			dodge.isRightBack = true;                  // 右(90°) → 後(180°)
			rate = (angle - quarter) / quarter;
		}
		else if (angle < -quarter)
		{
			dodge.isBackLeft = true;                   // 後(-180°) → 左(-90°)
			rate = (angle + DirectX::XM_PI) / quarter;
		}
		else
		{
			dodge.isLeftFront = true;                  // 左(-90°) → 前(0°)
			rate = (angle + quarter) / quarter;
		}

		dodge.blendRate = (std::max)(0.0f, (std::min)(rate, 1.0f));
	}
}

PlayerDodgeSystem::PlayerDodgeSystem()
{
	StateMachineSystem::RequireState<PlayerDodgeComponent>("Dodge", "PlayerDodgeSystem");
}

void PlayerDodgeSystem::Update(entt::registry& registry, float elapsedTime, const SceneContext& context)
{
	(void)context;

	auto view = registry.view<PlayerIntentComponent, PlayerDodgeComponent, StateMachineComponent, PlayerLocomotionComponent, TransformComponent>();
	
	view.each([&](PlayerIntentComponent& intent, PlayerDodgeComponent& dodge, const StateMachineComponent& stateMachine,
		PlayerLocomotionComponent& loco, const TransformComponent& transform)
		{

			if (!stateMachine.IsIn("Dodge")) return;

			// 入った瞬間 回避方向を決める
			if (stateMachine.enteredThisFrame)
			{
				// スティック入力があればその方向、無ければ体の後ろ
				float dodgeYaw = intent.wantsMove
					? PlayerMovementUtils::CalcFacingYaw(intent.moveDir.x, intent.moveDir.z)
					: transform.rotation.y + DirectX::XM_PI;
				dodge.dodgeDir = PlayerMovementUtils::YawToForward(dodgeYaw);

				// クリップの選択用：4方向は入った瞬間だけ決め、回避中は変えない
				dodge.direction = ClassifyDirection(PlayerMovementUtils::AngleDiff(transform.rotation.y, dodgeYaw));
				dodge.isFront = (dodge.direction == PlayerDodgeDirection::Front);
				dodge.isBack = (dodge.direction == PlayerDodgeDirection::Back);
				dodge.isLeft = (dodge.direction == PlayerDodgeDirection::Left);
				dodge.isRight = (dodge.direction == PlayerDodgeDirection::Right);

				dodge.elapsed = 0.0f;
				dodge.finished = false;

				// 入ったステートが入力を消費する
				intent.dodge.Consume();
			}
			// 回避中 スティック入力があれば、回避方向を入力のほうへ曲げる
			else if (intent.wantsMove)
			{
				float dodgeYaw = PlayerMovementUtils::CalcFacingYaw(dodge.dodgeDir.x, dodge.dodgeDir.z);
				float inputYaw = PlayerMovementUtils::CalcFacingYaw(intent.moveDir.x, intent.moveDir.z);
				dodgeYaw = PlayerMovementUtils::RotateTowards(dodgeYaw, inputYaw, dodge.turnSpeed, elapsedTime);
				dodge.dodgeDir = PlayerMovementUtils::YawToForward(dodgeYaw);
			}

			// 毎フレーム 時間を進め、体から見た回避方向を求める
			dodge.elapsed += elapsedTime;

			float localDiff = PlayerMovementUtils::AngleDiff(
				transform.rotation.y,
				PlayerMovementUtils::CalcFacingYaw(dodge.dodgeDir.x, dodge.dodgeDir.z));
			
			dodge.localX = sinf(localDiff);
			dodge.localZ = cosf(localDiff);
			// Blend 用のペアと割合
			UpdateBlendPair(dodge);

			// 終わりの判定：最低時間を過ぎてR2を離した、または最大時間に達した
			if (!dodge.finished)
			{
				bool released = !intent.dodge.held && dodge.elapsed >= dodge.minTime;
				bool timeUp = dodge.elapsed >= dodge.maxTime;
				dodge.finished = released || timeUp;
			}

			// 毎フレーム：Locomotion への指示
			LocomotionCommand& cmd = loco.command;
			cmd.mode = LocomotionMode::Directional;
			cmd.moveDir = dodge.dodgeDir;
			cmd.targetSpeed = dodge.dodgeSpeed;
			cmd.accelHalfLife = dodge.accelHalfLife;

			// 走り出す時は、抜ける前に向きを回避方向へ合わせておく
			cmd.faceMoveDir = dodge.finished && intent.wantsMove;
		});
}