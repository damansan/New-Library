#pragma once
#include <DirectXMath.h>
#include "Engine/ECS/Reflect.h"

struct PlayerSlideComponent
{
	REFLECT_BEGIN(PlayerSlideComponent, "Player Slide")

		// 滑り方
		REFLECT_FIELD(float, friction,       (5.0f), ("Friction",         0.0f, 30.0f)) // 減速速度
		REFLECT_FIELD(float, endSpeed,       (1.5f), ("End Speed",        0.0f, 10.0f)) // スライディング終了速度
		REFLECT_FIELD(float, steerTurnSpeed, (1.5f), ("Steer Turn Speed", 0.0f, 10.0f)) // 左右移動速度
		REFLECT_FIELD(float, accelHalfLife, (0.05f), ("Accel Half Life",  0.0f, 1.0f))  // 滑る速度に寄せる半減期
		REFLECT_FIELD(float, heightScale,    (0.5f), ("Height Scale",     0.1f, 1.0f))  // 滑っている間の当たり判定の高さ

		// 状態
		REFLECT_FIELD(float, slideSpeed,      (0.0f), ("Slide Speed",     0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 今の滑る速さ
		REFLECT_FIELD(bool,  wantsStand,     (false), ("Wants Stand",     0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 立ちたい
		REFLECT_FIELD(bool,  standRequested, (false), ("Stand Requested", 0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 立ちたい かつ 立てる
		REFLECT_FIELD(bool,  finished,       (false), ("Finished",        0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 滑り終わった

		REFLECT_END()

		// 内部実装用
		DirectX::XMFLOAT3 slideDir = { 0.0f, 0.0f, 1.0f }; // 滑る向き(水平、長さ1)
};