#include "CameraEffectSystem.h"

#include <cmath>

#include "Game/ECS/Components/PlayerLocomotionComponent.h"
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
		});
}

// プレイヤーを見て、きっかけを作る
CameraEffectTrigger CameraEffectSystem::BuildTrigger(entt::registry& registry, const CameraEffectComponent& effect, entt::entity player)
{
	CameraEffectTrigger trigger;
	if (player == entt::null) return trigger;

	const auto* loco = registry.try_get<PlayerLocomotionComponent>(player);
	if (loco)
	{
		trigger.speed = loco->horizontalSpeed;
	}

	trigger.running = trigger.speed >= effect.runSpeed;
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

	// 急に変わらないように、半減期で寄せる
	effect.fovAdd = PlayerMovementUtils::Damper(effect.fovAdd, targetAdd, effect.fovHalfLife, elapsedTime);

	// 出力に足す
	effect.fovOffset += effect.fovAdd;
}
// 走り出しに左右へ揺らす
void CameraEffectSystem::RunBobEffect(CameraEffectComponent& effect, const CameraEffectTrigger& trigger, const GameCameraComponent& cam, float elapsedTime)
{
	// 速度が一定以下になるまで再度揺らさない
	if (trigger.speed <= effect.runBobRearmSpeed)
	{
		effect.runBobArmed = true;
	}

	// 走り出しに揺らす
	if (effect.runBobArmed && trigger.running)
	{
		effect.runBobArmed = false;
		effect.runBobActive = true;
		effect.runBobTime = 0.0f;
	}
	if (!effect.runBobActive) return;

	// 一定時間で止める
	effect.runBobTime += elapsedTime;
	if (effect.runBobTime >= effect.runBobDuration)
	{
		effect.runBobActive = false;
		return;
	}


	// 揺れの量
	float phase = 0.0f;
	if (effect.runBobStepInterval > 0.0f)
	{
		phase = effect.runBobTime / effect.runBobStepInterval * DirectX::XM_PI;
	}
	const float sway = std::sin(phase);
	const float lift = std::sin(phase * 2.0f); // 横の倍の速さ → ∞
	const float remain = 1.0f - effect.runBobTime / effect.runBobDuration;
	const float envelope = remain * remain;
	const float amount = sway * envelope; // -1～+1（＋で右）
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

	// 上下も eye と target を同じだけ（向きは変えない）
	effect.eyeOffset.y += liftY;
	effect.targetOffset.y += liftY;

	effect.roll += amount * effect.runBobRollDeg;
}
