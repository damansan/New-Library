#pragma once
#include <DirectXMath.h>
#include "Engine/ECS/Reflect.h"

// 水平移動をどう計算するか
enum class LocomotionMode
{
	Ground,      // 地上：体の向きに沿って進む
	Air,         // 空中：速度のベクトルを保ち入力方向に寄せる
	Directional, // 向きとは別に進む
	External,    // 外部
};

// ステートから Locomotion への指示
struct LocomotionCommand
{
	LocomotionMode    mode = LocomotionMode::Ground;
	DirectX::XMFLOAT3 moveDir = { 0.0f, 0.0f, 0.0f }; // 向かいたい方向
	float  targetSpeed = 0.0f;    // 目標の速さ
	float  accelHalfLife = 0.05f; // 速さを上げる時の半減期 
	float  decelHalfLife = 0.06f; // 速さを下げる時の半減期
	float  turnSpeed = 0.0f;      // 旋回速度
	float  carryHalfLife = 0.2f;  // 持ち越しの速度が減る半減期
	bool   faceMoveDir = false;   // 体の向きを 向かいたい方向 に一気に向ける
	DirectX::XMFLOAT3 faceDir = { 0.0f, 0.0f, 0.0f }; // 体を向ける方向
};

// 水平移動の運転手と、体の状態
struct PlayerLocomotionComponent
{
	REFLECT_BEGIN(PlayerLocomotionComponent, "Player Locomotion")

		// 設定値
		REFLECT_FIELD(float, fallGraceTime, (0.1f), ("Fall Grace Time", 0.0f, 0.5f)) // 接地していない状態がこの時間続いたら、宙に浮いたとみなす

		// 体の状態
		REFLECT_FIELD(float, horizontalSpeed, (0.0f), ("Horizontal Speed", 0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 今の水平の速さ(持ち越しを含む)
		REFLECT_FIELD(float, verticalSpeed,   (0.0f), ("Vertical Speed",   0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 今の縦の速さ
		
		REFLECT_FIELD(float, landingHorizontalSpeed, (0.0f), ("Landing Horizontal Speed", 0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 空中にいた最後のフレームの水平の速さ
		REFLECT_FIELD(float, landingVerticalSpeed,   (0.0f), ("Landing Vertical Speed",   0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 空中にいた最後のフレームの縦の速さ
		
		REFLECT_FIELD(float, airTime,   (0.0f),  ("Air Time",    0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 接地していない時間(秒)
		REFLECT_FIELD(bool, isAirborne, (false), ("Is Airborne", 0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 宙に浮いているか(猶予付き)

		// 計算の途中経過
		REFLECT_FIELD(float, currentSpeed,   (0.0f), ("Current Speed", 0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // Ground：自分で走っている速さ
		REFLECT_FIELD(float, desiredSpeed,   (0.0f), ("Desired Speed", 0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 向きのずれを考慮した目標の速さ
	
		REFLECT_FIELD(float, facingErrorDeg, (0.0f), ("Facing Error Deg", 0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 体の向きと指示方向のずれ(度)

		REFLECT_END()

	// 内部実装用
	DirectX::XMFLOAT3 carryVelocity = { 0.0f, 0.0f, 0.0f }; // 持ち越しの速度
	LocomotionMode    lastMode = LocomotionMode::Ground;    // 前フレームのモード
	int               landingCount = 0;                     // 着地した回数

	// ステートからの指示
	LocomotionCommand command;
};