#pragma once
#include <DirectXMath.h>
#include "Engine/ECS/Reflect.h"

struct PlayerSlideComponent
{
	REFLECT_BEGIN(PlayerSlideComponent, "Player Slide")

		// 入った直後の加速
		REFLECT_FIELD(float, boostSpeed, (2.0f), ("Boost Speed", 0.0f, 10.0f)) // 入った直後に、この速さだけ上乗せする
		REFLECT_FIELD(float, boostTime,	 (0.2f), ("Boost Time", 0.0f, 1.0f))   // 上乗せにかける時間

		// 滑り方
		REFLECT_FIELD(float, friction,       (5.0f), ("Friction",       0.0f, 30.0f)) // 平地で、毎秒この速さずつ減る
		REFLECT_FIELD(float, endSpeed,       (1.5f), ("End Speed",      0.0f, 10.0f)) // この速さ以下になったら滑り終わる
		REFLECT_FIELD(float, strafeSpeed, (2.0f), ("Strafe Speed", 0.0f, 10.0f)) // スティックを真横に倒した時の、横へずれる速さ

		// 坂
		REFLECT_FIELD(float, slopeGravityScale, (1.0f), ("Slope Gravity Scale", 0.0f, 5.0f))  // 坂で加速・減速する強さ(重力に対する倍率)
		REFLECT_FIELD(float, maxSpeed, (15.0f), ("Max Speed", 0.0f, 50.0f)) // 滑る速さの上限
		REFLECT_FIELD(float, downhillTurnSpeed, (6.0f), ("Downhill Turn Speed", 0.0f, 30.0f)) // 坂の下る向きへ回る速さ(rad/s。坂の急さを掛ける)
		REFLECT_FIELD(float, uphillEndAngleDeg, (10.0f), ("Uphill End Angle Deg", 0.0f, 90.0f)) // 滑る向きに沿って、これより急な上り坂なら滑り終わる(度)
		REFLECT_FIELD(float, accelHalfLife, (0.05f), ("Accel Half Life",  0.0f, 1.0f))  // 滑る速度に寄せる半減期
		REFLECT_FIELD(float, heightScale,    (0.5f), ("Height Scale",     0.1f, 1.0f))  // 滑っている間の当たり判定の高さ

		// 状態
		REFLECT_FIELD(float, slideSpeed, (0.0f), ("Slide Speed", 0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 今の滑る速さ(前へ)
		REFLECT_FIELD(float, elapsed, (0.0f), ("Elapsed", 0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 入ってからの時間
		REFLECT_FIELD(bool,  wantsStand,     (false), ("Wants Stand",     0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 立ちたい
		REFLECT_FIELD(bool,  standRequested, (false), ("Stand Requested", 0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 立ちたい かつ 立てる
		REFLECT_FIELD(bool,  finished,       (false), ("Finished",        0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 滑り終わった

		REFLECT_END()

		// 内部実装用
		DirectX::XMFLOAT3 slideDir = { 0.0f, 0.0f, 1.0f }; // 滑る向き(水平、長さ1)
};