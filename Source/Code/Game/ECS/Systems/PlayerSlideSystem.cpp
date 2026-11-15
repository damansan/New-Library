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
				slide.elapsed = 0.0f;

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

			slide.elapsed += elapsedTime;

			// 坂：滑る向きを、坂の下る向きへ少しずつ回す(急な坂ほど速く回る)
			{
				const DirectX::XMFLOAT3& n = contact.groundNormal;
				const float steepness = sqrtf(n.x * n.x + n.z * n.z); // 坂の急さ(平地は 0)
				if (steepness > 0.05f)
				{
					// 坂の下る向き(水平、長さ1)
					const float downX = n.x / steepness;
					const float downZ = n.z / steepness;

					// 今の滑る向きから見て、下る向きが何度ずれているか(右が +)
					const float rightX = slide.slideDir.z;
					const float rightZ = -slide.slideDir.x;
					const float front = downX * slide.slideDir.x + downZ * slide.slideDir.z;
					const float side = downX * rightX + downZ * rightZ;
					const float angleToDown = atan2f(side, front);

					// 下る向きが後ろ側にある(上り坂を登っている)時は回さない(振り向かない)
					float maxStep = 0.0f;
					if (front > 0.0f)
					{
						// このフレームで回れる角度(行き過ぎない)
						maxStep = slide.downhillTurnSpeed * steepness * elapsedTime;
					}
					float step = angleToDown;
					if (step > maxStep)
					{
						step = maxStep;
					}
					if (step < -maxStep)
					{
						step = -maxStep;
					}

					// 滑る向きを step だけ右へ回す(マイナスなら左)
					const float c = cosf(step);
					const float s = sinf(step);
					float newX = slide.slideDir.x * c + rightX * s;
					float newZ = slide.slideDir.z * c + rightZ * s;
					const float len = sqrtf(newX * newX + newZ * newZ);
					if (len > 1e-4f)
					{
						slide.slideDir = { newX / len, 0.0f, newZ / len };
					}
				}
			}

			// 前への速さ：入った直後は少し加速、その後は摩擦で減速
			if (slide.elapsed <= slide.boostTime)
			{
				if (slide.boostTime > 0.0f)
				{
					slide.slideSpeed += slide.boostSpeed / slide.boostTime * elapsedTime;
				}
			}
			else
			{
				slide.slideSpeed -= slide.friction * elapsedTime;
			}

			// 坂：重力のうち、滑る向きに沿った分だけ加速(上り坂なら減速)
			// 地面に当たっていない時は法線が真上なので、0 になる
			const DirectX::XMFLOAT3& n = contact.groundNormal;
			const float gravity = fabsf(movement.gravity);
			const float alongSlide = n.x * slide.slideDir.x + n.z * slide.slideDir.z;
			slide.slideSpeed += gravity * n.y * alongSlide * slide.slopeGravityScale * elapsedTime;

			// 速さを 0 〜 maxSpeed に収める
			if (slide.slideSpeed < 0.0f)
			{
				slide.slideSpeed = 0.0f;
			}
			if (slide.slideSpeed > slide.maxSpeed)
			{
				slide.slideSpeed = slide.maxSpeed;
			}

			// 終わり：加速が終わった後、遅くなったら滑り終わる(→ しゃがみ)
			slide.finished = false;
			if (slide.elapsed > slide.boostTime && slide.slideSpeed <= slide.endSpeed)
			{
				slide.finished = true;
			}

			// 終わり：急な上り坂に滑り込んだら、すぐ滑り終わる(→ しゃがみ)
			if (contact.hitGround && alongSlide < -sinf(DirectX::XMConvertToRadians(slide.uphillEndAngleDeg)))
			{
				slide.finished = true;
			}

			// 左右の操作：滑る向きは変えず、横へずらす(前後に倒した分は使わない)
			// 滑る向きの右(前が +Z のとき、右は +X)
			const float rightX = slide.slideDir.z;
			const float rightZ = -slide.slideDir.x;
			float side = 0.0f;
			if (intent.wantsMove)
			{
				// 横の成分(右が +、左が -)
				side = intent.moveDir.x * rightX + intent.moveDir.z * rightZ;
			}

			// 立つ：立ちたい かつ 頭の上が空いている(低い隙間の中なら、抜けるまで待つ)
			slide.standRequested = false;
			if (slide.wantsStand && contact.canStand)
			{
				slide.standRequested = true;
			}

			// 毎フレーム：Locomotion への指示(前へ滑る ＋ 横へずれる)
			float vx = slide.slideDir.x * slide.slideSpeed + rightX * side * slide.strafeSpeed;
			float vz = slide.slideDir.z * slide.slideSpeed + rightZ * side * slide.strafeSpeed;
			const float targetSpeed = sqrtf(vx * vx + vz * vz);
			if (targetSpeed > 1e-4f)
			{
				vx /= targetSpeed;
				vz /= targetSpeed;
			}

			LocomotionCommand& cmd = loco.command;
			cmd.mode = LocomotionMode::Directional;
			cmd.moveDir = { vx, 0.0f, vz };
			cmd.targetSpeed = targetSpeed;
			cmd.accelHalfLife = slide.accelHalfLife;
			cmd.faceMoveDir = false;
			cmd.faceDir = slide.slideDir; // 体は滑る向きへ(横へずれる分は入れない)

			// 毎フレーム：当たり判定を低くする
			movement.heightScale = slide.heightScale;
		});
}