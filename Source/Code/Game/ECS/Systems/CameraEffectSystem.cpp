#include "CameraEffectSystem.h"

#include <cmath>

#include "Game/ECS/Components/PlayerLocomotionComponent.h"
#include "Game/ECS/Components/PlayerWallRunComponent.h"
#include "Engine/ECS/Components/MovementComponent.h"
#include "Engine/ECS/Components/StateMachineComponent.h"
#include "Game/ECS/Components/PlayerSlideComponent.h"
#include "Game/ECS/Systems/PlayerMovementUtils.h"

void CameraEffectSystem::Update(entt::registry& registry, float elapsedTime)
{
	auto view = registry.view<CameraEffectComponent, GameCameraComponent>();
	view.each([&](CameraEffectComponent& effect, const GameCameraComponent& cam)
		{
			// 出力を初期化
			effect.eyeOffset = { 0.0f, 0.0f, 0.0f };
			effect.targetOffset = { 0.0f, 0.0f, 0.0f };
			effect.fovOffset = 0.0f;
			effect.roll = 0.0f;

			// 追従先
			entt::entity player = entt::null;
			if (cam.followTarget != entt::null && registry.valid(cam.followTarget))
			{
				player = cam.followTarget;
			}

			// きっかけを作る
			const CameraEffectTrigger trigger = BuildTrigger(registry, effect, player);

			// 演出ごとの処理
			FovEffect(effect, trigger, elapsedTime);
			RunBobEffect(effect, trigger, cam, elapsedTime);
			KickShakeEffect(effect, trigger, cam, elapsedTime);
			LandShakeEffect(effect, trigger, cam, elapsedTime);
			SlideDropEffect(effect, trigger, elapsedTime);
			WallTiltEffect(effect, trigger, elapsedTime);
		});
}

// プレイヤーを見て、きっかけを作る
CameraEffectTrigger CameraEffectSystem::BuildTrigger(entt::registry& registry, const CameraEffectComponent& effect, entt::entity player)
{
	CameraEffectTrigger trigger;
	if (player == entt::null) return trigger;

	// 速度
	const auto* loco = registry.try_get<PlayerLocomotionComponent>(player);
	if (loco)
	{
		trigger.speed = loco->horizontalSpeed;
	}

	trigger.running = trigger.speed >= effect.runSpeed;

	const auto* stateMachine = registry.try_get<StateMachineComponent>(player);

	// 壁走り・壁蹴り
	const auto* wallRun = registry.try_get<PlayerWallRunComponent>(player);
	if (wallRun)
	{
		trigger.wallKicked = wallRun->kicked;
		trigger.wallRight = wallRun->isWallRight;
		if (stateMachine && stateMachine->IsIn("WallRun") && !wallRun->kicked)
		{
			trigger.wallRunning = true;
		}
	}

	// 落下速度
	const auto* movement = registry.try_get<MovementComponent>(player);
	if (movement)
	{
		trigger.velocityY = movement->velocity.y;
		trigger.isGround = movement->isGrounded;
	}

	// スライディング
	const auto* slide = registry.try_get<PlayerSlideComponent>(player);
	if (stateMachine && slide && stateMachine->IsIn("Slide"))
	{
		trigger.sliding = true;
		trigger.slideSpeed = slide->slideSpeed;
	}

	return trigger;
}

// 速さで Fov を広げる
void CameraEffectSystem::FovEffect(CameraEffectComponent& effect, const CameraEffectTrigger& trigger, float elapsedTime)
{
	// runSpeed～speedFull を 0～1 に
	float t = 0.0f;
	const float range = effect.speedFull - effect.runSpeed;
	if (range > 0.0f)
	{
		t = (trigger.speed - effect.runSpeed) / range;
	}
	if (t < 0.0f)
	{
		t = 0.0f;
	}
	if (t > 1.0f)
	{
		t = 1.0f;
	}
	const float targetAdd = effect.fovAddMax * t;

	// 半減期で寄せる
	effect.fovAdd = PlayerMovementUtils::Damper(effect.fovAdd, targetAdd, effect.fovHalfLife, elapsedTime);

	// 出力に足す
	effect.fovOffset += effect.fovAdd;
}
// 走り出しに左右へ揺らす
void CameraEffectSystem::RunBobEffect(CameraEffectComponent& effect, const CameraEffectTrigger& trigger, const GameCameraComponent& cam, float elapsedTime)
{
	PulseState& pulse = effect.runBob;

	// 速度が一定以下になるまで再度揺らさない
	if (trigger.speed <= effect.runBobRearmSpeed)
	{
		pulse.ready = true;
	}

	// 走り出しに揺らす
	if (pulse.ready && trigger.running)
	{
		pulse.ready = false;
		pulse.active = true;
		pulse.time = 0.0f;
	}
	if (!pulse.active) return;

	// 一定時間で止める
	pulse.time += elapsedTime;
	if (pulse.time >= effect.runBobDuration)
	{
		pulse.active = false;
		return;
	}

	// 揺れの量
	float phase = 0.0f;
	if (effect.runBobStepInterval > 0.0f)
	{
		phase = pulse.time / effect.runBobStepInterval * DirectX::XM_PI;
	}
	const float sway = std::sin(phase);
	const float lift = std::sin(phase * 2.0f); 
	const float remain = 1.0f - pulse.time / effect.runBobDuration;
	const float envelope = remain * remain;
	const float amount = sway * envelope; // +1 で右
	const float liftY = lift * envelope * effect.runBobLift;

	// カメラから見た右へ、eye と target を同じだけずらす
	const float fx = cam.base.currentTarget.x - cam.base.currentEye.x;
	const float fz = cam.base.currentTarget.z - cam.base.currentEye.z;
	const float len = std::sqrt(fx * fx + fz * fz);
	if (len > 0.0001f)
	{
		const float rightX = fz / len;
		const float rightZ = -fx / len;
		const float shift = amount * effect.runBobSway;

		effect.eyeOffset.x += rightX * shift;
		effect.eyeOffset.z += rightZ * shift;
		effect.targetOffset.x += rightX * shift;
		effect.targetOffset.z += rightZ * shift;
	}

	// 上下も eye と target を同じだけ
	effect.eyeOffset.y += liftY;
	effect.targetOffset.y += liftY;

	effect.roll += amount * effect.runBobRollDeg;
}
// 壁蹴りで小さく揺らす
void CameraEffectSystem::KickShakeEffect(CameraEffectComponent& effect, const CameraEffectTrigger& trigger, const GameCameraComponent& cam, float elapsedTime)
{
	PulseState& pulse = effect.kickShake;

	// 壁走りに入るまで再度揺らさない
	if (!trigger.wallKicked)
	{
		pulse.ready = true;
	}

	// 蹴った瞬間に揺らす
	if (pulse.ready && trigger.wallKicked)
	{
		pulse.ready = false;
		pulse.active = true;
		pulse.time = 0.0f;
	}
	if (!pulse.active) return;

	// 一定時間で止める
	pulse.time += elapsedTime;
	if (pulse.time >= effect.kickShakeDuration)
	{
		pulse.active = false;
		return;
	}

	// 揺れの量
	const float phase = pulse.time / effect.kickShakeDuration * DirectX::XM_PI;
	float dir = -1.0f; // 壁が左
	if (trigger.wallRight) 
	{
		dir = 1.0f;    // 壁が右
	}
	const float amount = std::sin(phase) * dir; // +1で右

	// カメラから見た右へ、eye と target を同じだけずらす
	const float fx = cam.base.currentTarget.x - cam.base.currentEye.x;
	const float fz = cam.base.currentTarget.z - cam.base.currentEye.z;
	const float len = std::sqrt(fx * fx + fz * fz);
	if (len > 0.0001f)
	{
		const float rightX = fz / len;
		const float rightZ = -fx / len;
		const float shift = amount * effect.kickShakeSway;

		effect.eyeOffset.x += rightX * shift;
		effect.eyeOffset.z += rightZ * shift;
		effect.targetOffset.x += rightX * shift;
		effect.targetOffset.z += rightZ * shift;
	}

	effect.roll += amount * effect.kickShakeRollDeg;
}
// 着地で揺らす
void CameraEffectSystem::LandShakeEffect(CameraEffectComponent& effect, const CameraEffectTrigger& trigger, const GameCameraComponent& cam, float elapsedTime)
{
	PulseState& pulse = effect.landShake;

	// 素早く落ちるまで再度揺らさない
	if (!trigger.isGround)
	{
		pulse.ready = false;
		if (trigger.velocityY <= -effect.landFallSpeed)
		{
			pulse.ready = true;
		}
	}

	// 着地時に揺らす
	if (pulse.ready && trigger.isGround)
	{
		pulse.ready = false;
		pulse.active = true;
		pulse.time = 0.0f;
	}
	if (!pulse.active) return;

	// 一定時間で止める
	pulse.time += elapsedTime;
	if (pulse.time >= effect.landShakeDuration)
	{
		pulse.active = false;
		return;
	}

	// 揺れの量：２回揺らす
	const float phase = pulse.time / effect.landShakeDuration * DirectX::XM_2PI;
	const float remain = 1.0f - pulse.time / effect.landShakeDuration;
	const float envelope = remain * remain;
	const float amount = std::sin(phase) * envelope; // +1で右

	// カメラから見た右へ、eye と target を同じだけずらす
	const float fx = cam.base.currentTarget.x - cam.base.currentEye.x;
	const float fz = cam.base.currentTarget.z - cam.base.currentEye.z;
	const float len = std::sqrt(fx * fx + fz * fz);
	if (len > 0.0001f)
	{
		const float rightX = fz / len;
		const float rightZ = -fx / len;
		const float shift = amount * effect.landShakeSway;

		effect.eyeOffset.x += rightX * shift;
		effect.eyeOffset.z += rightZ * shift;
		effect.targetOffset.x += rightX * shift;
		effect.targetOffset.z += rightZ * shift;
	}

	effect.roll += amount * effect.landShakeRollDeg;
}
// スライド中は下げ続ける
void CameraEffectSystem::SlideDropEffect(CameraEffectComponent& effect, const CameraEffectTrigger& trigger, float elapsedTime)
{
	// スライド中は 1 へ、それ以外は 0 へ、一定の速さで進める
	float step = 1.0f;
	if (effect.slideDropTime > 0.0f)
	{
		step = elapsedTime / effect.slideDropTime;
	}
	if (trigger.sliding)
	{
		effect.slideDropRate += step;
		if (effect.slideDropRate > 1.0f)
		{
			effect.slideDropRate = 1.0f;
		}
	}
	else
	{
		effect.slideDropRate -= step;
		if (effect.slideDropRate < 0.0f)
		{
			effect.slideDropRate = 0.0f;
		}
	}
	if (effect.slideDropRate <= 0.0f) return;

	// sin の山の前半で下げる
	const float dropY = -std::sin(effect.slideDropRate * DirectX::XM_PIDIV2) * effect.slideDrop;

	// eye と target を同じだけ下げる
	effect.eyeOffset.y += dropY;
	effect.targetOffset.y += dropY;
}
// 壁走り中傾ける
void CameraEffectSystem::WallTiltEffect(CameraEffectComponent& effect, const CameraEffectTrigger& trigger, float elapsedTime)
{
	// 目標と、傾けていく途中かを決める
	float target = 0.0f;
	bool  growing = false;
	if (trigger.wallRunning)
	{
		float dir = -1.0f; // 壁が左
		if (trigger.wallRight)
		{
			dir = 1.0f;    // 壁が右
		}
		target = dir * effect.wallTiltMaxDeg;

		// 反対側に傾いている間は、まず素早く 0 へ
		if (effect.wallTilt * dir >= 0.0f)
		{
			growing = true;
		}
	}

	// 段々弱く傾ける
	if (growing)
	{
		effect.wallTilt = PlayerMovementUtils::Damper(effect.wallTilt, target, effect.wallTiltHalfLife, elapsedTime);
		effect.roll += effect.wallTilt;
		return;
	}

	// 戻す
	const float step = effect.wallTiltReturnSpeed * elapsedTime;
	if (effect.wallTilt < target) 
	{
		effect.wallTilt += step;
		if (effect.wallTilt > target)
		{
			effect.wallTilt = target;
		}
	}
	else
	{
		effect.wallTilt -= step;
		if (effect.wallTilt < target)
		{
			effect.wallTilt = target;
		}
	}

	// 出力に足す
	effect.roll += effect.wallTilt;
}
