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

    // 追従パラメータ
	float distance = 5.0f;     // カメラと対象の距離
	float height = 2.0f;       // カメラの高さ
	float yaw = 0.0f;          // カメラの回転角度
	float pitch = 0.3f;        // カメラの上下角度
	float followSpeed = 10.0f; // 追従速度

	// ロックオンパラメータ
	float lockOnSide = 1.0f;        // +1.0で右/-1.0で左
	float sidewaysStrength = 6.0f;  // 横方向のずれの強さ
	DirectX::XMFLOAT3 lockOnTargetOffset = { 0.0f, 1.0f, 0.0f }; // 注視対象のオフセット
	float lockOnTargetRatio = 0.5f;
};