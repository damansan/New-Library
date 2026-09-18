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

// ゲームカメラコンポーネント
struct GameCameraComponent
{
    // カメラの共通データ（eye, target）
    BaseCameraComponent base;  

	// カメラの種類
    Mode mode = Mode::Follow;

    // 追従対象の Entity
    entt::entity followTarget = entt::null;
	// 注視対象の Entity
	entt::entity lookAtTarget = entt::null;

    // パラメータ
	float yaw = 0.0f;          // カメラの回転角度
	float pitch = 0.3f;        // カメラの上下角度

	// 追従カメラのパラメーター
	float followCameraDistance   = 4.4f;  // 追従カメラの距離
	float followCameraHeight     = 1.7f;  // 追従カメラの高さ
	float followCameraTargetYUp  = 1.6f;  // 注視点のオフセット(Y)
	float followCameraEyeSpeedRatio    = 6.0f;  //  eye   の追従速度比率
	float followCameraTargetSpeedRatio = 12.0f; // target の追従速度比率

};