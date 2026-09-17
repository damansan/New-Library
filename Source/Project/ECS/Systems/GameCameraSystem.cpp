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

			// 初回だけ currentEye / currentTarget を即座に設定
			if (cam.base.currentEye.x == 0.0f &&
				cam.base.currentEye.y == 0.0f &&
				cam.base.currentEye.z == 0.0f)
			{
				cam.base.currentTarget = followTransform->position;
				cam.base.currentEye =
				{
					followTransform->position.x,
					followTransform->position.y + cam.height,
					followTransform->position.z - cam.distance
				};
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

// Follow用のcurrentEye計算
void GameCameraSystem::UpdateFollowEye(
	GameCameraComponent& cam,
	const TransformComponent& followTransform,
	float elapsedTime)
{
	DirectX::XMFLOAT3 basePos = followTransform.position;

	float sinYaw = sinf(cam.yaw);
	float cosYaw = cosf(cam.yaw);
	float cosPitch = cosf(cam.pitch);
	float sinPitch = sinf(cam.pitch);

	DirectX::XMFLOAT3 idealEye =
	{
		basePos.x - sinYaw * cosPitch * cam.distance,
		basePos.y + sinPitch * cam.distance + cam.height,
		basePos.z - cosYaw * cosPitch * cam.distance
	};

	float t = 1.0f - powf(0.001f, elapsedTime * cam.followSpeed);

	cam.base.currentEye.x += (idealEye.x - cam.base.currentEye.x) * t;
	cam.base.currentEye.y += (idealEye.y - cam.base.currentEye.y) * t;
	cam.base.currentEye.z += (idealEye.z - cam.base.currentEye.z) * t;
}
// LookAt用のcurrentEye計算
void GameCameraSystem::UpdateLockOnEye(GameCameraComponent& cam, entt::registry& registry, float elapsedTime)
{
	DirectX::XMFLOAT3 playerPos = registry.get<TransformComponent>(cam.followTarget).position;
	DirectX::XMFLOAT3 enemyPos = registry.get<TransformComponent>(cam.lookAtTarget).position;

	// 敵→プレイヤー、のXZ平面上の方向ベクトル(後方方向)
	float backX = playerPos.x - enemyPos.x;
	float backZ = playerPos.z - enemyPos.z;
	float len = sqrtf(backX * backX + backZ * backZ);

	DirectX::XMFLOAT3 idealEye = cam.base.currentEye; // 万一lenが0の場合、今の位置を維持
	if (len > 0.0001f)
	{
		backX /= len;
		backZ /= len;

		// 後方ベクトルを90度回転させた、横方向のベクトル(Y軸を上として、右向き)
		float sideX = -backZ;
		float sideZ = backX;

		// 敵→現在のカメラ、のベクトルとの外積で、毎フレーム左右を再判定する
		float toCamX = cam.base.currentEye.x - enemyPos.x;
		float toCamZ = cam.base.currentEye.z - enemyPos.z;
		float cross = backX * toCamZ - backZ * toCamX;
		cam.lockOnSide = (cross >= 0.0f) ? 1.0f : -1.0f;

		// 敵との距離に応じて、回転させる角度を決める(近いほど、大きく回り込む)
		// atan2fを使うことで、「横へのずれ量」を、直接「角度」に変換する
		float sidewaysAmount = cam.sidewaysStrength / len;
		float rotateAngle = atan2f(sidewaysAmount, cam.distance) * cam.lockOnSide;

		// 後方ベクトルを、rotateAngleの分だけ回転させる(XZ平面上の回転)
		float cosA = cosf(rotateAngle);
		float sinA = sinf(rotateAngle);
		float rotatedX = backX * cosA - backZ * sinA;
		float rotatedZ = backX * sinA + backZ * cosA;

		// 回転させた方向へ、常にdistance分だけ進んだ位置(=常に円周上)
		idealEye =
		{
			playerPos.x + rotatedX * cam.distance,
			playerPos.y + cam.height,
			playerPos.z + rotatedZ * cam.distance
		};
	}

	float t = 1.0f - powf(0.001f, elapsedTime * cam.followSpeed);

	cam.base.currentEye.x += (idealEye.x - cam.base.currentEye.x) * t;
	cam.base.currentEye.y += (idealEye.y - cam.base.currentEye.y) * t;
	cam.base.currentEye.z += (idealEye.z - cam.base.currentEye.z) * t;
}

// Followカメラの更新
void GameCameraSystem::UpdateFollowTarget(
	GameCameraComponent& cam,
	const TransformComponent& followTransform,
	float elapsedTime)
{
	DirectX::XMFLOAT3 targetPos = followTransform.position;
	float t = 1.0f - powf(0.001f, elapsedTime * cam.followSpeed);

	cam.base.currentTarget.x += (targetPos.x - cam.base.currentTarget.x) * t;
	cam.base.currentTarget.y += ((targetPos.y + cam.height * 0.5f) - cam.base.currentTarget.y) * t;
	cam.base.currentTarget.z += (targetPos.z - cam.base.currentTarget.z) * t;
}
// LookAtカメラの更新
void GameCameraSystem::UpdateLookAtTarget(
	GameCameraComponent& cam,
	entt::registry& registry,
	float elapsedTime)
{
	DirectX::XMFLOAT3 playerPos = registry.get<TransformComponent>(cam.followTarget).position;
	DirectX::XMFLOAT3 enemyPos = registry.get<TransformComponent>(cam.lookAtTarget).position;

	// プレイヤーと敵の間を、指定の割合で補間した位置を、注視点の基準にする
	DirectX::XMFLOAT3 targetPos =
	{
		playerPos.x + (enemyPos.x - playerPos.x) * cam.lockOnTargetRatio,
		playerPos.y + (enemyPos.y - playerPos.y) * cam.lockOnTargetRatio,
		playerPos.z + (enemyPos.z - playerPos.z) * cam.lockOnTargetRatio
	};

	// 注視点に、任意のオフセットを加える
	targetPos.x += cam.lockOnTargetOffset.x;
	targetPos.y += cam.lockOnTargetOffset.y;
	targetPos.z += cam.lockOnTargetOffset.z;

	float t = 1.0f - powf(0.001f, elapsedTime * cam.followSpeed);

	cam.base.currentTarget.x += (targetPos.x - cam.base.currentTarget.x) * t;
	cam.base.currentTarget.y += (targetPos.y - cam.base.currentTarget.y) * t;
	cam.base.currentTarget.z += (targetPos.z - cam.base.currentTarget.z) * t;
}