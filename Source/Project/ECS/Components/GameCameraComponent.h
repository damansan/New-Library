#pragma once
#include <DirectXMath.h>
#include "entt.hpp"
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
	float yaw = 0.0f;          // カメラの回転角度
	float pitch = 0.3f;        // カメラの上下角度


	// 追従カメラのパラメーター
	float followCameraDistance = 4.4f;   // カメラの距離
	float followCameraHeight = 1.7f;     // カメラの高さ
	float followCameraTargetYUp = 1.6f;  // 注視点のオフセット(Y)
	float followCameraEyeSpeedRatio = 12.0f;    // eyeの追従速度比率
	float followCameraTargetSpeedRatio = 24.0f; // target追従速度比率


	// 注視カメラのパラメーター
	float lockOnCameraEyeSpeedRatio = 12.0f;    // eyeの追従速度比率
	float lockOnCameraTargetSpeedRatio = 15.0f; // target追従速度比率

	// 近距離 / 遠距離 切り替えの距離しきい値
	float lockOnRangeSwitchDistance = 10.0f;

	// 近距離時のカメラパラメーター
	float lockOnNearCameraDistance = 6.0f;   // カメラの距離
	float lockOnNearCameraHeight = 3.0f;     // カメラの高さ
	float lockOnNearCameraTargetYUp = 2.0f;  // 注視点のオフセット(Y)
	float lockOnNearAngleOffsetMax = 30.0f;  // Enemyに最も近づいた時の、デフォルト角度のズレ

	float lockOnSide = 1.0f;                 // +1=右 -1=左
	float lockOnSideSwitchThreshold = 0.35f; // この角度を超えて倒すと向きが切り替わる

	float lockOnSideSmooth = 1.0f;           // lockOnSideへ、lerpでなめらかに追従する値
	float lockOnSideSmoothSpeed = 2.0f;      // 1フレームで縮める割合の目安値

	// 遠距離時のカメラパラメーター
	float lockOnFarCameraDistance = 6.0f;  // カメラの距離
	float lockOnFarCameraHeight = 3.0f;    // カメラの高さ
	float lockOnFarCameraTargetYUp = 2.0f; // 注視点のオフセット(Y)


	// 円の中心
	float lockOnCenterRatio = 0.25f;          // PlayerからEnemyの按分比率
	float lockOnFarCenterMaxDistance = 1.2f;  // Playerからの固定距離の上限(遠距離)
	float lockOnTargetCircleRadiusMax = 3.0f; // target円の半径の上限
	
	// 円周上の角度(水平)
	float lockOnOrbitAngle = 0.0f;
	float lockOnOrbitRotateSpeed = 2.0f; // スティックでの回転速度
	// 円周上の角度(上下)
	float lockOnOrbitPitch = 0.0f;
	float lockOnOrbitPitchRotateSpeed = 2.0f; // スティックでの上下回転速度
	
	// スティック入力のない時に戻す力
	float lockOnOrbitReturnSpeed = 2.0f;


	// 減衰ばね用パラメーター(カメラの速度)
	DirectX::XMFLOAT3 eyeVelocity = {};    // currentEyeの現在の速度
	DirectX::XMFLOAT3 targetVelocity = {}; // currentTargetの現在の速度

	// 減衰ばね用パラメーター(eye)  
	float eyeSpringStiffness = 200.0f; // バネの強さ（大きいほど速く反応する）
	float eyeSpringDamping = 20.0f;    // 減衰（小さいほどオーバーシュートが大きく、揺れが長く続く）
};