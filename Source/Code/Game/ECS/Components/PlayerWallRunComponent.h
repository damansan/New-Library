#pragma once
#include <DirectXMath.h>
#include "Engine/ECS/Reflect.h"

struct PlayerWallRunComponent
{
	REFLECT_BEGIN(PlayerWallRunComponent, "Player Wall Run")

		// 入る条件
		REFLECT_FIELD(float, enterMaxAngleDeg, (40.0f), ("Enter Max Angle Deg", 0.0f, 90.0f)) // 当たる角度がこれ以下なら入れる
		REFLECT_FIELD(float, enterMinSpeed,    (4.0f),  ("Enter Min Speed",     0.0f, 20.0f)) // 水平の速さがこれ以上なら入れる

		// 走り方
		REFLECT_FIELD(float, minRunSpeed,   (7.0f), ("Min Run Speed", 0.0f, 20.0f))  // 壁を走る速さの下限
		REFLECT_FIELD(float, accelHalfLife, (0.1f), ("Accel Half Life", 0.0f, 1.0f)) // 走る速度に寄せる半減期
		REFLECT_FIELD(float, stickSpeed,    (1.0f), ("Stick Speed", 0.0f, 5.0f))     // 壁へ押し付ける速さ

		// 縦の動き
		REFLECT_FIELD(float, maxEnterVerticalSpeed, (3.0f),  ("Max Enter Vertical Speed", -10.0f, 10.0f)) // 入った瞬間の縦の速さの上限
		REFLECT_FIELD(float, minEnterVerticalSpeed, (-1.0f), ("Min Enter Vertical Speed", -10.0f, 10.0f)) // 入った瞬間の縦の速さの下限
		REFLECT_FIELD(float, gravityScale, (0.3f), ("Gravity Scale", 0.0f, 1.0f))    // 壁走り中の重力の倍率

		// 壁を蹴る
		REFLECT_FIELD(float, kickOutSpeed, (4.0f), ("Kick Out Speed", 0.0f, 20.0f)) // 壁から離れる向きの速さ
		REFLECT_FIELD(float, kickUpSpeed, (7.0f), ("Kick Up Speed", 0.0f, 20.0f)) // 上向きの速さ
		REFLECT_FIELD(float, kickForwardScale, (1.6f), ("Kick Forward Scale", 0.0f, 3.0f))  // 走っていた速さ × これ を前へ(1 より大きいと加速)
		REFLECT_FIELD(float, kickLockTime, (0.2f), ("Kick Lock Time", 0.0f, 1.0f))  // 蹴った後、スティックが効かない時間

		// 状態
		REFLECT_FIELD(bool,  isWallRight, (false), ("Is Wall Right", 0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 入った時に決めた左右
		REFLECT_FIELD(float, elapsed,      (0.0f), ("Elapsed",       0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 入ってからの時間
		REFLECT_FIELD(bool, kicked, (false), ("Kicked", 0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 壁を蹴った
		REFLECT_FIELD(float, kickTime, (0.0f), ("Kick Time", 0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 蹴ってからの時間
		REFLECT_FIELD(bool, finished, (false), ("Finished", 0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 壁走りが終わった


		REFLECT_FIELD(float, radius, (0.5f), ("Radius", 0.0f, 2.0f)) // 壁走り中の当たり判定の半径

		REFLECT_END()

		// 内部実装用
		DirectX::XMFLOAT3 wallNormal = { 0.0f, 0.0f, 0.0f }; // 今沿っている壁の法線
	DirectX::XMFLOAT3 runDir = { 0.0f, 0.0f, 0.0f }; // 壁に沿って進む向き
	float             runSpeed = 0.0f;                 // 走る速さ
};