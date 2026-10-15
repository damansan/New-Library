#pragma once
#include <DirectXMath.h>
#include "Engine/ECS/Reflect.h"

// 水平移動を誰が動かすか
enum class LocomotionMode
{
	Drive,    // Locomotion が速さと向きを動かす
	External, // 外部(ルートモーションなど)に任せる。Locomotion は実際の速さを写すだけ
};

// ステートから Locomotion への指示
struct LocomotionCommand
{
	LocomotionMode    mode = LocomotionMode::Drive;
	DirectX::XMFLOAT3 moveDir = { 0.0f, 0.0f, 0.0f }; // 向かいたい方向(長さ1)。長さ0なら向きを変えない

	float             targetSpeed = 0.0f;             // 目標の速さ(m/s)

	float             accelHalfLife = 0.05f; // 速さを上げる時の半減期(秒)
	float             decelHalfLife = 0.06f; // 速さを下げる時の半減期(秒)

	float             turnSpeed = 0.0f;  // 旋回速度(rad/s)。0なら回らない
};

// 水平移動の運転手
struct PlayerLocomotionComponent
{
	REFLECT_BEGIN(PlayerLocomotionComponent, "Player Locomotion")

		// 状態
		REFLECT_FIELD(float, currentSpeed,   (0.0f), ("Current Speed",    0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 運転手が管理している速さ
		REFLECT_FIELD(float, desiredSpeed,   (0.0f), ("Desired Speed",    0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 向きのずれを考慮した目標の速さ
		REFLECT_FIELD(float, facingErrorDeg, (0.0f), ("Facing Error Deg", 0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 体の向きと指示方向のずれ(度)

		REFLECT_END()

		// ステートからの指示。PlayerLocomotionSystem が使ったら初期値に戻す
		LocomotionCommand command;
};