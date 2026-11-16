#include "CameraEffectSystem.h"

#include "Game/ECS/Components/CameraEffectComponent.h"
#include "Game/ECS/Components/GameCameraComponent.h"
#include "Game/ECS/Components/PlayerLocomotionComponent.h"
#include "Game/ECS/Systems/PlayerMovementUtils.h"

void CameraEffectSystem::Update(entt::registry& registry, float elapsedTime)
{
	auto view = registry.view<CameraEffectComponent, GameCameraComponent>();
	view.each([&](CameraEffectComponent& effect, const GameCameraComponent& cam)
		{
			// 初めて動いた時は、いきなり基本の FOV から始める
			if (effect.currentFov <= 0.0f)
			{
				effect.currentFov = effect.baseFov;
			}

			// プレイヤーの速さ(見つからなければ 0 ＝ 基本の FOV)
			float speed = 0.0f;
			if (cam.followTarget != entt::null && registry.valid(cam.followTarget))
			{
				const auto* loco = registry.try_get<PlayerLocomotionComponent>(cam.followTarget);
				if (loco)
				{
					speed = loco->horizontalSpeed;
				}
			}

			// 速さ → 0〜1 の割合 → 目標の FOV
			float t = 0.0f;
			const float range = effect.speedFull - effect.speedStart;
			if (range > 0.0f)
			{
				t = (speed - effect.speedStart) / range;
			}
			if (t < 0.0f)
			{
				t = 0.0f;
			}
			if (t > 1.0f)
			{
				t = 1.0f;
			}
			effect.targetFov = effect.baseFov + (effect.maxFov - effect.baseFov) * t;

			// 急に変わらないように、半減期で寄せる
			effect.currentFov = PlayerMovementUtils::Damper(effect.currentFov, effect.targetFov, effect.fovHalfLife, elapsedTime);
		});
}

void CameraEffectSystem::ApplyToCamera(entt::registry& registry, Camera& camera)
{
	auto view = registry.view<CameraEffectComponent>();
	for (auto entity : view)
	{
		const auto& effect = view.get<CameraEffectComponent>(entity);
		if (effect.currentFov <= 0.0f) continue;

		camera.SetFovDeg(effect.currentFov);
		break; // カメラは1つだけ(GameCamera の ApplyToCamera と同じ)
	}
}

// 速さで Fov を広げる
void CameraEffectSystem::FovEffect(entt::registry& registry, CameraEffectComponent& effect, entt::entity player, float elapsedTime)
{
}
