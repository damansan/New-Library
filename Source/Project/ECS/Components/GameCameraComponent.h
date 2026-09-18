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
	float followCameraTargetYUp = 1.0f;   // 注視点のオフセット(Y)
	float followCameraSpeedRatio = 5.5f; // 追従速度の比率
	float followCameraHeight = 0.5f;      // 追従カメラの高さ
	float followCameraDistance = 3.0f;    // 追従カメラの距離

	// 追従対象の速度計算用
	DirectX::XMFLOAT3 prevFollowTargetPos = {}; // 前フレームの、追従対象の位置
	float followTargetSpeed = 0.0f;             // 追従対象の速度
};