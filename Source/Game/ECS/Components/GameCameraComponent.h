#pragma once
#include <DirectXMath.h>
#include "entt.hpp"
#include "Engine/ECS/Reflect.h"
#include "Engine/ECS/Components/BaseCameraComponent.h"

// カメラの種類
enum class Mode
{
	Follow, // 追従カメラ
	LookAt, // 注視カメラ
};
// 注視カメラの距離状態
enum class LockOnRangeState
{
	Near, // 近距離：くの字
	Far,  // 遠距離：縦並び
};

// ゲームカメラコンポーネント
struct GameCameraComponent
{
	// 調整用カメラパラメーター
	REFLECT_BEGIN(GameCameraComponent, "GameCamera")

		// 追従カメラ
		REFLECT_FIELD(float, followCameraDistance, (4.4f), ("Follow Distance"))
		REFLECT_FIELD(float, followCameraHeight, (1.7f), ("Follow Height"))
		REFLECT_FIELD(float, followCameraTargetYUp, (1.6f), ("Follow Target Y Up"))
		REFLECT_FIELD(float, followCameraEyeSpeedRatio, (12.0f), ("Follow Eye Speed Ratio"))
		REFLECT_FIELD(float, followCameraTargetSpeedRatio, (24.0f), ("Follow Target Speed Ratio"))

		// ロックオン共通
		REFLECT_FIELD(float, lockOnCameraEyeSpeedRatio, (12.0f), ("LockOn Eye Speed Ratio"))
		REFLECT_FIELD(float, lockOnCameraTargetSpeedRatio, (15.0f), ("LockOn Target Speed Ratio"))
		REFLECT_FIELD(float, lockOnRangeSwitchDistance, (10.0f), ("LockOn Range Switch Distance"))

		// ロックオン近距離
		REFLECT_FIELD(float, lockOnNearCameraDistance, (6.0f), ("Near Distance"))
		REFLECT_FIELD(float, lockOnNearCameraHeight, (3.0f), ("Near Height"))
		REFLECT_FIELD(float, lockOnNearCameraTargetYUp, (2.0f), ("Near Target Y Up"))
		REFLECT_FIELD(float, lockOnNearAngleOffsetMax, (30.0f), ("Near Angle Offset Max"))

		// ロックオン遠距離
		REFLECT_FIELD(float, lockOnFarCameraDistance, (6.0f), ("Far Distance"))
		REFLECT_FIELD(float, lockOnFarCameraHeight, (3.0f), ("Far Height"))
		REFLECT_FIELD(float, lockOnFarCameraTargetYUp, (2.0f), ("Far Target Y Up"))

		// くの字の向き
		REFLECT_FIELD(float, lockOnSideSwitchThreshold, (0.35f), ("Side Switch Threshold"))
		REFLECT_FIELD(float, lockOnSideSmoothSpeed, (2.0f), ("Side Smooth Speed"))

		// 円の中心O
		REFLECT_FIELD(float, lockOnCenterRatio, (0.25f), ("Center Ratio"))
		REFLECT_FIELD(float, lockOnFarCenterMaxDistance, (1.2f), ("Far Center Max Distance"))
		REFLECT_FIELD(float, lockOnTargetCircleRadiusMax, (3.0f), ("Target Circle Radius Max"))

		// 円周上の回転（スティック操作）
		REFLECT_FIELD(float, lockOnOrbitRotateSpeed, (2.0f), ("Orbit Rotate Speed"))
		REFLECT_FIELD(float, lockOnOrbitPitchRotateSpeed, (2.0f), ("Orbit Pitch Rotate Speed"))
		REFLECT_FIELD(float, lockOnOrbitReturnSpeed, (2.0f), ("Orbit Return Speed"))

		// 減衰ばね（Y軸）
		REFLECT_FIELD(float, eyeSpringStiffness, (200.0f), ("Spring Stiffness"))
		REFLECT_FIELD(float, eyeSpringDamping, (20.0f), ("Spring Damping"))

	REFLECT_END()

	// カメラの共通データ（eye, target）
	BaseCameraComponent base;

	// カメラの種類
	Mode mode = Mode::Follow;
	// 注視カメラの距離状態
	LockOnRangeState lockOnRange = LockOnRangeState::Near;

	// 追従対象の Entity
	entt::entity followTarget = entt::null;
	// 注視対象の Entity
	entt::entity lookAtTarget = entt::null;

	// パラメータ
	float yaw = 0.0f;   // カメラの回転角度
	float pitch = 0.3f; // カメラの上下角度
	
	// ロックオン中の、円周上の角度（スティックで動く）
	float lockOnOrbitAngle = 0.0f;
	float lockOnOrbitPitch = 0.0f;

	// くの字の向き（+1=右 -1=左）と、それになめらかに追従する値
	float lockOnSide = 1.0f;
	float lockOnSideSmooth = 1.0f;

	// ばね計算用の現在速度
	DirectX::XMFLOAT3 eyeVelocity = {};
	DirectX::XMFLOAT3 targetVelocity = {};
};