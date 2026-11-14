#include "PlayerSlideSystem.h"
#include <cmath>

#include "Game/ECS/Components/PlayerIntentComponent.h"
#include "Game/ECS/Components/PlayerSlideComponent.h"
#include "Game/ECS/Components/PlayerLocomotionComponent.h"
#include "Game/ECS/Systems/PlayerMovementUtils.h"

#include "Engine/ECS/Components/StateMachineComponent.h"
#include "Engine/ECS/Components/MovementComponent.h"
#include "Engine/ECS/Components/TransformComponent.h"
#include "Engine/ECS/Components/CharacterContactComponent.h"
#include "Engine/ECS/Systems/StateMachineSystem.h"

PlayerSlideSystem::PlayerSlideSystem()
{
	StateMachineSystem::RequireState<PlayerSlideComponent>("Slide", "PlayerSlideSystem");
}

void PlayerSlideSystem::Update(entt::registry& registry, float elapsedTime, const SceneContext& context)
{
	(void)context;

	auto view = registry.view<PlayerIntentComponent, PlayerSlideComponent, StateMachineComponent,
		PlayerLocomotionComponent, MovementComponent, TransformComponent, CharacterContactComponent>();

	view.each([&](PlayerIntentComponent& intent, PlayerSlideComponent& slide, const StateMachineComponent& stateMachine,
		PlayerLocomotionComponent& loco, MovementComponent& movement, const TransformComponent& transform,
		const CharacterContactComponent& contact)
		{
			if (!stateMachine.IsIn("Slide")) return;

			if (stateMachine.enteredThisFrame)
			{
				// 入った瞬間：入ってきた ○ を使い切る
				intent.dodge.Consume();
				slide.wantsStand = false;
				slide.standRequested = false;
				slide.finished = false;

				// 滑る向きと速さ：今の速度を引き継ぐ
				const float vx = movement.velocity.x;
				const float vz = movement.velocity.z;
				const float speed = sqrtf(vx * vx + vz * vz);
				if (speed > 1e-4f)
				{
					slide.slideDir = { vx / speed, 0.0f, vz / speed };
				}
				else
				{
					// 止まっている：体の向きへ滑る
					slide.slideDir = PlayerMovementUtils::YawToForward(transform.rotation.y);
				}
				slide.slideSpeed = speed;
			}
			else if (intent.dodge.buffered)
			{
				// 滑っている途中で ○：立ちたい。使い切る
				slide.wantsStand = true;
				intent.dodge.Consume();
			}

			// 左右の操作：スティックの「滑る向きに対して横の成分」だけで、向きを少しずつ回す
			if (intent.wantsMove)
			{
				// 滑る向きの右(前が +Z のとき、右は +X)
				const float rightX = slide.slideDir.z;
				const float rightZ = -slide.slideDir.x;

				// 横の成分(右が +、左が -、前後に倒した分は 0)
				const float side = intent.moveDir.x * rightX + intent.moveDir.z * rightZ;
				const float angle = side * slide.steerTurnSpeed * elapsedTime;

				// 向きを angle だけ右へ回す
				const float c = cosf(angle);
				const float s = sinf(angle);
				const float newX = slide.slideDir.x * c + rightX * s;
				const float newZ = slide.slideDir.z * c + rightZ * s;
				const float len = sqrtf(newX * newX + newZ * newZ);
				if (len > 1e-4f)
				{
					slide.slideDir = { newX / len, 0.0f, newZ / len };
				}
			}

			// 減速(平地)：毎秒 friction ずつ減らす
			slide.slideSpeed -= slide.friction * elapsedTime;
			if (slide.slideSpeed < 0.0f)
			{
				slide.slideSpeed = 0.0f;
			}

			// 終わり：遅くなったら滑り終わる(→ しゃがみ)
			slide.finished = false;
			if (slide.slideSpeed <= slide.endSpeed)
			{
				slide.finished = true;
			}

			// 立つ：立ちたい かつ 頭の上が空いている(低い隙間の中なら、抜けるまで待つ)
			slide.standRequested = false;
			if (slide.wantsStand && contact.canStand)
			{
				slide.standRequested = true;
			}

			// 毎フレーム：Locomotion への指示(滑る向き × 速さ)
			LocomotionCommand& cmd = loco.command;
			cmd.mode = LocomotionMode::Directional;
			cmd.moveDir = slide.slideDir;
			cmd.targetSpeed = slide.slideSpeed;
			cmd.accelHalfLife = slide.accelHalfLife;
			cmd.faceMoveDir = true;

			// 毎フレーム：当たり判定を低くする
			movement.heightScale = slide.heightScale;
		});
}