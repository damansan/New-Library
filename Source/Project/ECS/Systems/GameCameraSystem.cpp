#include "GameCameraSystem.h"
#include <algorithm>

void GameCameraSystem::Update(entt::registry& registry, float elapsedTime)
{
	auto view = registry.view<GameCameraComponent, TransformComponent>();
	view.each([&](auto entity, GameCameraComponent& cam, TransformComponent& transform)
		{
			// 追従対象が未設定なら何も行わない
			if (cam.followTarget == entt::null) return;    
			if (!registry.valid(cam.followTarget)) return;  

			// 追従対象のTransformComponentを取得、なければ何も行わない
			auto* followTransform = registry.try_get<TransformComponent>(cam.followTarget);
			if (!followTransform) return;

			// 初回だけ currentEye / currentTarget / prevFollowTargetPos を即座に設定
			if (cam.base.currentEye.x == 0.0f &&
				cam.base.currentEye.y == 0.0f &&
				cam.base.currentEye.z == 0.0f)
			{
				cam.base.currentTarget = followTransform->position;
				cam.base.currentEye =
				{
					followTransform->position.x,
					followTransform->position.y + cam.followCameraHeight,
					followTransform->position.z - cam.followCameraDistance
				};
				cam.prevFollowTargetPos = followTransform->position;
			}

			// 追従対象の速度を計算する
			{
				DirectX::XMFLOAT3 delta =
				{
					followTransform->position.x - cam.prevFollowTargetPos.x,
					followTransform->position.y - cam.prevFollowTargetPos.y,
					followTransform->position.z - cam.prevFollowTargetPos.z
				};
				float dist = sqrtf(delta.x * delta.x + delta.y * delta.y + delta.z * delta.z);

				cam.followTargetSpeed = (elapsedTime > 0.0f) ? (dist / elapsedTime) : 0.0f;
				cam.prevFollowTargetPos = followTransform->position;
			}

			// InputComponent からカメラ操作入力を取得
			auto* input = registry.try_get<GameInputComponent>(cam.followTarget);
			if (input)
			{
				const float rotSpeed = 2.0f;
				cam.yaw += input->cameraStick.x * rotSpeed * elapsedTime;   // 左右スティックでカメラ回転
				cam.pitch += input->cameraStick.y * rotSpeed * elapsedTime; // 上下スティックでカメラ上下角度
				cam.pitch = std::clamp(cam.pitch, -0.8f, 1.0f); // 上下角度の制限

				// L2を押した瞬間、Follow<->LookAtをトグル切り替え
				if (input->lockOnTrigger.pressed)
				{
					if (cam.mode == Mode::Follow && registry.valid(cam.lookAtTarget))
					{
						cam.mode = Mode::LookAt;
					}
					else if (cam.mode == Mode::LookAt)
					{
						cam.mode = Mode::Follow;
					}
				}
			}

			// LookAt中に、注視対象が破棄されていたら、Followへ自動的に戻す
			if (cam.mode == Mode::LookAt && !registry.valid(cam.lookAtTarget))
			{
				cam.mode = Mode::Follow;
			}


			// 注視点をカメラのモードに応じて変更
			switch (cam.mode)
			{
			case Mode::Follow: // 追従対象自身を見る
				UpdateFollowEye(cam, *followTransform, elapsedTime);
				UpdateFollowTarget(cam, *followTransform, elapsedTime);
				break;
			case Mode::LookAt: // ロックオン先を見る
				UpdateLockOnEye(cam, registry, elapsedTime);
				UpdateLookAtTarget(cam, registry, elapsedTime);
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
		auto& cam = view.get<GameCameraComponent>(entity);

		// followTarget が未設定ならスキップ
		if (cam.followTarget == entt::null) continue;

		// eye と target が同じ位置ならスキップ
		if (cam.base.currentEye.x == cam.base.currentTarget.x &&
			cam.base.currentEye.y == cam.base.currentTarget.y &&
			cam.base.currentEye.z == cam.base.currentTarget.z) continue;

		camera.SetLookAt(
			cam.base.currentEye,
			cam.base.currentTarget,
			{ 0.0f, 1.0f, 0.0f }
		);
		break;
	}
}

// Followカメラの更新
void GameCameraSystem::UpdateFollowTarget(GameCameraComponent& cam, const TransformComponent& followTransform, float elapsedTime)
{
	DirectX::XMFLOAT3 targetPos = followTransform.position;

	float t = std::clamp(cam.followCameraSpeedRatio / 100.0f, 0.0f, 1.0f);

	cam.base.currentTarget.x += (targetPos.x - cam.base.currentTarget.x) * t;
	cam.base.currentTarget.y += ((targetPos.y + cam.followCameraTargetYUp) - cam.base.currentTarget.y) * t;
	cam.base.currentTarget.z += (targetPos.z - cam.base.currentTarget.z) * t;
}
// Follow用のcurrentEye計算
void GameCameraSystem::UpdateFollowEye(GameCameraComponent& cam, const TransformComponent& followTransform, float elapsedTime)
{
	DirectX::XMFLOAT3 basePos = followTransform.position;

	float sinYaw = sinf(cam.yaw);
	float cosYaw = cosf(cam.yaw);
	float cosPitch = cosf(cam.pitch);
	float sinPitch = sinf(cam.pitch);

	DirectX::XMFLOAT3 idealEye =
	{
		basePos.x - sinYaw * cosPitch * cam.followCameraDistance,
		basePos.y + sinPitch * cam.followCameraDistance + cam.followCameraHeight,
		basePos.z - cosYaw * cosPitch * cam.followCameraDistance
	};

	float t = std::clamp(cam.followCameraSpeedRatio / 100.0f, 0.0f, 1.0f);

	cam.base.currentEye.x += (idealEye.x - cam.base.currentEye.x) * t;
	cam.base.currentEye.y += (idealEye.y - cam.base.currentEye.y) * t;
	cam.base.currentEye.z += (idealEye.z - cam.base.currentEye.z) * t;
}

// LookAtカメラの更新
void GameCameraSystem::UpdateLookAtTarget(GameCameraComponent& cam, entt::registry& registry, float elapsedTime)
{
}
// LookAt用のcurrentEye計算
void GameCameraSystem::UpdateLockOnEye(GameCameraComponent& cam, entt::registry& registry, float elapsedTime)
{
}
