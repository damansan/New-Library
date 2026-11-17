#include "CameraEffectSystem.h"

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

			// 
			entt::entity player = entt::null;
			if (cam.followTarget != entt::null && registry.valid(cam.followTarget))
			{
				player = cam.followTarget;
			}

			// 演出ごとの処理
			FovEffect(registry, effect, player, elapsedTime);
		});
}

// 速さで Fov を広げる
void CameraEffectSystem::FovEffect(entt::registry& registry, CameraEffectComponent& effect, entt::entity player, float elapsedTime)
{
	// プレイヤーの速さ
	float speed = 0.0f;
	if (player != entt::null)
	{
		const auto* loco = registry.try_get<PlayerLocomotionComponent>(player);
		if (loco)
		{
			speed = loco->horizontalSpeed;
		}
	}

	// 
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
	const float targetAdd = effect.fovAddMax * t;

	// 急に変わらないように、半減期で寄せる
	effect.fovAdd = PlayerMovementUtils::Damper(effect.fovAdd, targetAdd, effect.fovHalfLife, elapsedTime);

	// 出力に足す
	effect.fovOffset += effect.fovAdd;
}
