#include "GameCameraSystem.h"
#include <algorithm>
#include <cmath>

#include "Engine/ECS/EntityFinder.h"
#include "Game/ECS/Components/GameInputComponent.h"

// 更新
void GameCameraSystem::Update(entt::registry& registry, float elapsedTime)
{
	auto view = registry.view<GameCameraComponent>();
	view.each([&](GameCameraComponent& cam)
		{
			// 追従対象が消えていたら、空に戻す
			if (cam.followTarget != entt::null && !registry.valid(cam.followTarget))
			{
				cam.followTarget = entt::null;
			}

			// 空なら Player を探す
			if (cam.followTarget == entt::null)
			{
				cam.followTarget = FindFirstByCategory(registry, EntityCategory::Player);
				cam.initialized = false;
			}
			if (cam.followTarget == entt::null) return;

			const auto* followTransform = registry.try_get<TransformComponent>(cam.followTarget);
			if (!followTransform) return;

			// 右スティックで回す
			const auto* input = registry.try_get<GameInputComponent>(cam.followTarget);
			if (input)
			{
				cam.yaw += input->cameraStick.x * cam.rotateSpeed * elapsedTime;
				cam.pitch += input->cameraStick.y * cam.rotateSpeed * elapsedTime;
				cam.pitch = std::clamp(cam.pitch, cam.pitchMin, cam.pitchMax);
			}

			// 最初・追従対象が変わった時：補間せずに正しい位置へ置く
			if (!cam.initialized)
			{
				cam.base.currentEye = CalcFollowIdealEye(cam, *followTransform);
				cam.base.currentTarget = CalcFollowIdealTarget(cam, *followTransform);
				cam.eyeVelocityY = 0.0f;
				cam.initialized = true;
			}

			// モードごとの処理
			switch (cam.mode)
			{
			case GameCameraMode::Follow:
				UpdateFollow(cam, *followTransform, elapsedTime);
				break;
			}
		});
}

// Camera クラスに結果を反映
void GameCameraSystem::ApplyToCamera(entt::registry& registry, Camera& camera)
{
	auto view = registry.view<GameCameraComponent>();
	for (auto entity : view)
	{
		const auto& cam = view.get<GameCameraComponent>(entity);
		if (!cam.initialized) continue;

		camera.SetLookAt(cam.base.currentEye, cam.base.currentTarget, { 0.0f, 1.0f, 0.0f });
		break;
	}
}

// 追従カメラの処理
void GameCameraSystem::UpdateFollow(GameCameraComponent& cam, const TransformComponent& followTransform, float elapsedTime)
{
	const DirectX::XMFLOAT3 idealEye = CalcFollowIdealEye(cam, followTransform);
	const DirectX::XMFLOAT3 idealTarget = CalcFollowIdealTarget(cam, followTransform);

	// target XYZ lerp 補間
	const float targetT = std::clamp(cam.targetLerpRatio / 100.0f, 0.0f, 1.0f);
	cam.base.currentTarget.x += (idealTarget.x - cam.base.currentTarget.x) * targetT;
	cam.base.currentTarget.y += (idealTarget.y - cam.base.currentTarget.y) * targetT;
	cam.base.currentTarget.z += (idealTarget.z - cam.base.currentTarget.z) * targetT;

	// eye XZ lerp 補間
	const float eyeT = std::clamp(cam.eyeLerpRatio / 100.0f, 0.0f, 1.0f);
	cam.base.currentEye.x += (idealEye.x - cam.base.currentEye.x) * eyeT;
	cam.base.currentEye.z += (idealEye.z - cam.base.currentEye.z) * eyeT;

	// eye ばね 補間
	const float accel = cam.springStiffness * (idealEye.y - cam.base.currentEye.y) - cam.springDamping * cam.eyeVelocityY;
	cam.eyeVelocityY += accel * elapsedTime;
	cam.base.currentEye.y += cam.eyeVelocityY * elapsedTime;
}

// 追従カメラの理想の eye
DirectX::XMFLOAT3 GameCameraSystem::CalcFollowIdealEye(const GameCameraComponent& cam, const TransformComponent& followTransform) const
{
	const DirectX::XMFLOAT3& p = followTransform.position;
	const float sinYaw = sinf(cam.yaw);
	const float cosYaw = cosf(cam.yaw);
	const float sinPitch = sinf(cam.pitch);
	const float cosPitch = cosf(cam.pitch);

	return {
		p.x - sinYaw * cosPitch * cam.followDistance,
		p.y + sinPitch * cam.followDistance + cam.followHeight,
		p.z - cosYaw * cosPitch * cam.followDistance
	};
}
// 追従カメラの理想の target
DirectX::XMFLOAT3 GameCameraSystem::CalcFollowIdealTarget(const GameCameraComponent& cam, const TransformComponent& followTransform) const
{
	const DirectX::XMFLOAT3& p = followTransform.position;
	return { p.x, p.y + cam.followTargetYUp, p.z };
}

