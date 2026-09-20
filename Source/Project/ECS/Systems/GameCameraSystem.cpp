#include "GameCameraSystem.h"
#include <algorithm>

void GameCameraSystem::Initialize(ID3D11Device* device)
{
	shapeRenderer = ShapeRenderer(device);
}

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
					followTransform->position.y + cam.followCameraHeight,
					followTransform->position.z - cam.followCameraDistance
				};
			}

			// InputComponent からカメラ操作入力を取得
			auto* input = registry.try_get<GameInputComponent>(cam.followTarget);
			if (input)
			{
				if (cam.mode == Mode::Follow)
				{
					// Follow カメラ
					const float rotSpeed = 2.0f;
					cam.yaw += input->cameraStick.x * rotSpeed * elapsedTime;   // スティック左右でカメラ回転
					cam.pitch += input->cameraStick.y * rotSpeed * elapsedTime; // スティック上下でカメラ上下角度
					cam.pitch = std::clamp(cam.pitch, -0.8f, 1.0f); // 上下角度の制限
				}
				else // LockOn カメラ
				{
					// スティック入力有
					if (input->cameraStick.hasInput)
					{
						// スティック左右でカメラ回転
						cam.lockOnOrbitAngle -= input->cameraStick.x * cam.lockOnOrbitRotateSpeed * elapsedTime;
						cam.lockOnOrbitPitch += input->cameraStick.y * cam.lockOnOrbitPitchRotateSpeed * elapsedTime;
					}
					else // スティック入力無
					{
						// 戻る強さの倍率、Enemyに近いほど弱くする
						float returnScale = 1.0f;
						if (registry.valid(cam.lookAtTarget))
						{
							DirectX::XMFLOAT3 enemyPos = registry.get<TransformComponent>(cam.lookAtTarget).position;
							float dx = followTransform->position.x - enemyPos.x;
							float dz = followTransform->position.z - enemyPos.z;
							float distToEnemy = sqrtf(dx * dx + dz * dz);
							returnScale = std::clamp(distToEnemy / cam.lockOnFarCenterMaxDistance, 0.0f, 1.0f);
						}

						// lerpでふわっと戻す
						float returnT = std::clamp(cam.lockOnOrbitReturnSpeed * returnScale * elapsedTime, 0.0f, 1.0f);
						cam.lockOnOrbitAngle -= cam.lockOnOrbitAngle * returnT;
						cam.lockOnOrbitPitch -= cam.lockOnOrbitPitch * returnT;
					}

					// -180度～180度の範囲に抑えて最短距離で戻すようにする
					cam.lockOnOrbitAngle = fmodf(cam.lockOnOrbitAngle + DirectX::XM_PI, DirectX::XM_2PI);
					if (cam.lockOnOrbitAngle < 0.0f) cam.lockOnOrbitAngle += DirectX::XM_2PI;
					cam.lockOnOrbitAngle -= DirectX::XM_PI;

					cam.lockOnOrbitPitch = std::clamp(cam.lockOnOrbitPitch, -0.8f, 0.8f); // 上下角度の制限
				
					// くの字が曲がる向きの判定（ヒステリシス）
					// 閾値を超えて倒した時だけ切り替わり、それ以外は今の向きを維持する
					if (cam.lockOnOrbitAngle > cam.lockOnSideSwitchThreshold)
					{
						cam.lockOnSide = 1.0f;
					}
					else if (cam.lockOnOrbitAngle < -cam.lockOnSideSwitchThreshold)
					{
						cam.lockOnSide = -1.0f;
					}

					// lockOnSideへ、lerpでなめらかに追従させる
					float sideT = std::clamp(cam.lockOnSideSmoothSpeed * elapsedTime, 0.0f, 1.0f);
					cam.lockOnSideSmooth += (cam.lockOnSide - cam.lockOnSideSmooth) * sideT;
				}

				// L2を押した瞬間、Follow<->LookAtをトグル切り替え
				if (input->lockOnTrigger.pressed)
				{
					if (cam.mode == Mode::Follow && registry.valid(cam.lookAtTarget))
					{
						cam.mode = Mode::LookAt;
					}
					else if (cam.mode == Mode::LookAt)
					{
						// LockOnからFollowに戻す際に逆算して同期させる
						float dx = cam.base.currentEye.x - followTransform->position.x;
						float dy = cam.base.currentEye.y - followTransform->position.y - cam.followCameraHeight;
						float dz = cam.base.currentEye.z - followTransform->position.z;
						float horizDist = sqrtf(dx * dx + dz * dz);

						if (horizDist > 0.0001f)
						{
							cam.yaw = atan2f(-dx, -dz);
							cam.pitch = std::clamp(atan2f(dy, horizDist), -0.8f, 1.0f);
						}

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

		// Camera クラスに eye と target を反映
		camera.SetLookAt(
			cam.base.currentEye,
			cam.base.currentTarget,
			{ 0.0f, 1.0f, 0.0f }
		);
		break;
	}
}

// デバッグ描画
void GameCameraSystem::DrawDebug(
	entt::registry& registry,
	ID3D11DeviceContext* dc, 
	const DirectX::XMFLOAT4X4& view,
	const DirectX::XMFLOAT4X4& projection, 
	const DirectX::XMFLOAT3& lightDirection)
{
	const DirectX::XMFLOAT4 eyeColor        = { 0.2f, 0.5f, 1.0f, 1.0f }; // eye         ：青
	const DirectX::XMFLOAT4 targetColor     = { 1.0f, 0.6f, 0.1f, 1.0f }; // target      ：オレンジ
	const DirectX::XMFLOAT4 centerColor     = { 1.0f, 1.0f, 1.0f, 1.0f }; // 中心 O      ：白
	const DirectX::XMFLOAT4 ringColor       = { 0.2f, 0.5f, 1.0f, 0.6f }; // 円周        ：薄い青
	const DirectX::XMFLOAT4 targetRingColor = { 0.6f, 0.3f, 1.0f, 0.6f }; // targetの円周：薄い紫

	auto camView = registry.view<GameCameraComponent>();
	for (auto entity : camView)
	{
		auto& cam = camView.get<GameCameraComponent>(entity);
		if (cam.mode != Mode::LookAt) continue;

		// 現在のeye/target
		shapeRenderer.DrawSphere(cam.base.currentEye, 0.15f, eyeColor);
		shapeRenderer.DrawSphere(cam.base.currentTarget, 0.15f, targetColor);

		if (registry.valid(cam.followTarget) && registry.valid(cam.lookAtTarget)) 
		{
			DirectX::XMFLOAT3 playerPos = registry.get<TransformComponent>(cam.followTarget).position;
			DirectX::XMFLOAT3 enemyPos = registry.get<TransformComponent>(cam.lookAtTarget).position;

			// EnemyからPlayerの方向ベクトル
			float backX = playerPos.x - enemyPos.x;
			float backZ = playerPos.z - enemyPos.z;
			float len = sqrtf(backX * backX + backZ * backZ);

			if (len <= 0.0001f) continue;


			backX /= len;
			backZ /= len;

			// 円の中心 O
			float centerDistByRatio = len * cam.lockOnCenterRatio;
			float centerDist = (std::min)(centerDistByRatio, cam.lockOnFarCenterMaxDistance);

			DirectX::XMFLOAT3 center =
			{
				playerPos.x - backX * centerDist,
				playerPos.y,
				playerPos.z - backZ * centerDist
			};

			shapeRenderer.DrawSphere(center, 0.15f, centerColor);

			// 近距離/遠距離で、eyeの半径・高さ・targetの高さを切り替える
			float eyeRadius, eyeHeight, targetYUp;
			if (cam.lockOnRange == LockOnRangeState::Near)
			{
				eyeRadius = cam.lockOnNearCameraDistance;
				eyeHeight = cam.lockOnNearCameraHeight;
				targetYUp = cam.lockOnNearCameraTargetYUp;
			}
			else
			{
				eyeRadius = cam.lockOnFarCameraDistance;
				eyeHeight = cam.lockOnFarCameraHeight;
				targetYUp = cam.lockOnFarCameraTargetYUp;

			}

			// デフォルト角度のオフセット
			float nearT = 1.0f - std::clamp(len / cam.lockOnRangeSwitchDistance, 0.0f, 1.0f);
			float angleOffset = DirectX::XMConvertToRadians(cam.lockOnNearAngleOffsetMax) * nearT * cam.lockOnSideSmooth;

			// eyeの円周を、点の集まりで描く
			const int segments = 32;
			for (int i = 0; i < segments; ++i)
			{
				float t = (float)i / (float)segments * DirectX::XM_2PI + angleOffset;
				DirectX::XMFLOAT3 p =
				{
					center.x + sinf(t) * eyeRadius,
					center.y + eyeHeight,
					center.z + cosf(t) * eyeRadius
				};
				shapeRenderer.DrawSphere(p, 0.05f, ringColor);
			}

			// targetの円周を、点の集まりで描く
			const float targetRadius = (std::min)(len * 0.25f, cam.lockOnTargetCircleRadiusMax);
			for (int i = 0; i < segments; ++i)
			{
				float t = (float)i / (float)segments * DirectX::XM_2PI + angleOffset;
				DirectX::XMFLOAT3 p =
				{
					center.x + sinf(t) * targetRadius,
					center.y + targetYUp,
					center.z + cosf(t) * targetRadius
				};
				shapeRenderer.DrawSphere(p, 0.05f, targetRingColor);
			}
		}
	}

	shapeRenderer.Render(dc, view, projection, lightDirection);
}

// Followカメラの更新
void GameCameraSystem::UpdateFollowTarget(GameCameraComponent& cam, const TransformComponent& followTransform, float elapsedTime)
{
	DirectX::XMFLOAT3 targetPos = followTransform.position;

	// targetの補間速度を正規化
	float t = std::clamp(cam.followCameraTargetSpeedRatio / 100.0f, 0.0f, 1.0f);

	// X,Zはlerpで補間、YはfollowCameraTargetYUpを加算してlerpで補間
	cam.base.currentTarget.x += (targetPos.x - cam.base.currentTarget.x) * t;
	cam.base.currentTarget.y += ((targetPos.y + cam.followCameraTargetYUp) - cam.base.currentTarget.y) * t;
	cam.base.currentTarget.z += (targetPos.z - cam.base.currentTarget.z) * t;
}
// Follow用のcurrentEye計算
void GameCameraSystem::UpdateFollowEye(GameCameraComponent& cam, const TransformComponent& followTransform, float elapsedTime)
{
	DirectX::XMFLOAT3 basePos = followTransform.position;

	// yaw, pitchから理想のeye位置を計算
	float sinYaw = sinf(cam.yaw);
	float cosYaw = cosf(cam.yaw);
	float cosPitch = cosf(cam.pitch);
	float sinPitch = sinf(cam.pitch);

	// FollowTargetからDistanceだけ離れた球面上の点を求める
	DirectX::XMFLOAT3 idealEye =
	{
		basePos.x - sinYaw * cosPitch * cam.followCameraDistance,
		basePos.y + sinPitch * cam.followCameraDistance + cam.followCameraHeight,
		basePos.z - cosYaw * cosPitch * cam.followCameraDistance
	};

	// eyeの補間速度を正規化
	float t = std::clamp(cam.followCameraEyeSpeedRatio / 100.0f, 0.0f, 1.0f);

	// X,Zはlerpで補間
	cam.base.currentEye.x += (idealEye.x - cam.base.currentEye.x) * t;
	cam.base.currentEye.z += (idealEye.z - cam.base.currentEye.z) * t;

	// 減衰ばね
	auto springAxis = [&](float current, float ideal, float& velocity) -> float
		{
			// 加速度 = バネの力（目標との差） - ダンパーの力（速度に比例して減衰）
			float accel = cam.eyeSpringStiffness * (ideal - current) - cam.eyeSpringDamping * velocity;
			velocity += accel * elapsedTime;
			return current + velocity * elapsedTime;
		};

	// Yは減衰ばねで補間
	cam.base.currentEye.y = springAxis(cam.base.currentEye.y, idealEye.y, cam.eyeVelocity.y);
}

// LookAtカメラの更新
void GameCameraSystem::UpdateLookAtTarget(GameCameraComponent& cam, entt::registry& registry, float elapsedTime)
{
	DirectX::XMFLOAT3 playerPos = registry.get<TransformComponent>(cam.followTarget).position;
	DirectX::XMFLOAT3 enemyPos = registry.get<TransformComponent>(cam.lookAtTarget).position;

	// EnemyからPlayer方向のベクトル算出
	float backX = playerPos.x - enemyPos.x;
	float backZ = playerPos.z - enemyPos.z;
	float len = sqrtf(backX * backX + backZ * backZ);

	DirectX::XMFLOAT3 idealTarget = cam.base.currentTarget;

	if (len > 0.0001f)
	{
		backX /= len;
		backZ /= len;

		// 円の中心 O
		float centerDistByRatio = len * cam.lockOnCenterRatio;
		float centerDist = (std::min)(centerDistByRatio, cam.lockOnFarCenterMaxDistance);

		DirectX::XMFLOAT3 center =
		{
			playerPos.x - backX * centerDist,
			playerPos.y,
			playerPos.z - backZ * centerDist
		};

		// targetの円の半径
		float targetRadius = (std::min)(len * 0.25f, cam.lockOnTargetCircleRadiusMax);

		// targetの高さ(近距離/遠距離で切り替え)
		float targetYUp = (cam.lockOnRange == LockOnRangeState::Near)
			? cam.lockOnNearCameraTargetYUp
			: cam.lockOnFarCameraTargetYUp;

		// 距離に応じてなめらかに変化
		float nearT = 1.0f - std::clamp(len / cam.lockOnRangeSwitchDistance, 0.0f, 1.0f);
		float angleOffset = DirectX::XMConvertToRadians(cam.lockOnNearAngleOffsetMax) * nearT * cam.lockOnSideSmooth;

		// Cameraと同じ角度で回転させた方向を求め、反対方向に配置する
		float angle = cam.lockOnOrbitAngle + angleOffset;
		float cosA = cosf(angle);
		float sinA = sinf(angle);
		float rotatedX = backX * cosA - backZ * sinA;
		float rotatedZ = backX * sinA + backZ * cosA;

		// orbitPitchも、eyeとは上下逆方向にすることで、点対称を保つ
		float cosP = cosf(cam.lockOnOrbitPitch);
		float sinP = sinf(cam.lockOnOrbitPitch);

		idealTarget =
		{
			center.x - rotatedX * targetRadius * cosP,
			center.y + targetYUp - targetRadius * sinP,
			center.z - rotatedZ * targetRadius * cosP
		};
	}

	// targetの補間速度を正規化
	float t = std::clamp(cam.lockOnCameraTargetSpeedRatio / 100.0f, 0.0f, 1.0f);

	// X,Zはlerpで補間
	cam.base.currentTarget.x += (idealTarget.x - cam.base.currentTarget.x) * t;
	cam.base.currentTarget.z += (idealTarget.z - cam.base.currentTarget.z) * t;

	// 減衰ばね
	auto springAxis = [&](float current, float ideal, float& velocity) -> float
		{
			float accel = cam.eyeSpringStiffness * (ideal - current) - cam.eyeSpringDamping * velocity;
			velocity += accel * elapsedTime;
			return current + velocity * elapsedTime;
		};

	// Yは減衰ばねで補間
	cam.base.currentTarget.y = springAxis(cam.base.currentTarget.y, idealTarget.y, cam.targetVelocity.y);
}
// LookAt用のcurrentEye計算
void GameCameraSystem::UpdateLockOnEye(GameCameraComponent& cam, entt::registry& registry, float elapsedTime)
{
	DirectX::XMFLOAT3 playerPos = registry.get<TransformComponent>(cam.followTarget).position;
	DirectX::XMFLOAT3 enemyPos = registry.get<TransformComponent>(cam.lookAtTarget).position;

	// EnemyからPlayer方向のベクトル算出
	float backX = playerPos.x - enemyPos.x;
	float backZ = playerPos.z - enemyPos.z;
	float len = sqrtf(backX * backX + backZ * backZ);

	// EnemyとPlayerが同じ位置の場合は現在の位置を維持する
	DirectX::XMFLOAT3 idealEye = cam.base.currentEye;
	if (len > 0.0001f)
	{
		// 正規化
		backX /= len;
		backZ /= len;
	
		// 距離に応じて 近距離 / 遠距離 を切り替える
		cam.lockOnRange = (len >= cam.lockOnRangeSwitchDistance)
			? LockOnRangeState::Far
			: LockOnRangeState::Near;

		// 近距離 / 遠距離 で、半径・高さと、デフォルト角度のオフセットを切り替える
		float eyeRadius, eyeHeight;
		if (cam.lockOnRange == LockOnRangeState::Near)
		{
			eyeRadius = cam.lockOnNearCameraDistance;
			eyeHeight = cam.lockOnNearCameraHeight;
		}
		else
		{
			eyeRadius = cam.lockOnFarCameraDistance;
			eyeHeight = cam.lockOnFarCameraHeight;
		}

		// デフォルト角度のoffsetEnemyに近づく程滑らかに角度を増やす
		float nearT = 1.0f - std::clamp(len / cam.lockOnRangeSwitchDistance, 0.0f, 1.0f);
		float angleOffset = DirectX::XMConvertToRadians(cam.lockOnNearAngleOffsetMax) * nearT * cam.lockOnSideSmooth;

		// 円の中心Oを求める
		float centerDistByRatio = len * cam.lockOnCenterRatio;
		float centerDist = (std::min)(centerDistByRatio, cam.lockOnFarCenterMaxDistance);

		DirectX::XMFLOAT3 center =
		{
			playerPos.x - backX * centerDist,
			playerPos.y,
			playerPos.z - backZ * centerDist
		};

		// backベクトル（Enemyの反対方向＝角度0の基準）を、(orbitAngle + angleOffset)ぶん回転させる
		float angle = cam.lockOnOrbitAngle + angleOffset;
		float cosA = cosf(angle);
		float sinA = sinf(angle);
		float rotatedX = backX * cosA - backZ * sinA;
		float rotatedZ = backX * sinA + backZ * cosA;

		// orbitPitchで、水平方向の広がり(cosP)と、上下方向の高さ(sinP)に分配する
		float cosP = cosf(cam.lockOnOrbitPitch);
		float sinP = sinf(cam.lockOnOrbitPitch);

		idealEye =
		{
			center.x + rotatedX * eyeRadius * cosP,
			center.y + eyeHeight + eyeRadius * sinP,
			center.z + rotatedZ * eyeRadius * cosP
		};
		
	}

	// eyeの補間速度を正規化
	float t = std::clamp(cam.lockOnCameraEyeSpeedRatio / 100.0f, 0.0f, 1.0f);

	// X,Zはlerpで補間
	cam.base.currentEye.x += (idealEye.x - cam.base.currentEye.x) * t;
	cam.base.currentEye.z += (idealEye.z - cam.base.currentEye.z) * t;

	// 減衰ばね
	auto springAxis = [&](float current, float ideal, float& velocity) -> float
		{
			float accel = cam.eyeSpringStiffness * (ideal - current) - cam.eyeSpringDamping * velocity;
			velocity += accel * elapsedTime;
			return current + velocity * elapsedTime;
		};

	// Yは減衰ばねで補間
	cam.base.currentEye.y = springAxis(cam.base.currentEye.y, idealEye.y, cam.eyeVelocity.y);
}
