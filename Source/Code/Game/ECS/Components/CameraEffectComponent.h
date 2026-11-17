#pragma once
#include <DirectXMath.h>
#include "Engine/ECS/Reflect.h"

// カメラの演出
struct CameraEffectComponent
{
	REFLECT_BEGIN(CameraEffectComponent, "Camera Effect")

		// 発火条件
		REFLECT_FIELD(float, runSpeed, (5.0f), ("Run Speed", 0.0f, 50.0f)) // この速さ以上でエフェクトを開始する

		// Fov：速さで広げる
		REFLECT_FIELD(float, fovAddMax, (15.0f), ("Fov Add Max", 0.0f, 60.0f)) // 速い時に足す FOV(度)
		REFLECT_FIELD(float, speedFull, (15.0f), ("Speed Full", 0.0f, 50.0f)) // この速さで fovAddMax になる(m/s)
		REFLECT_FIELD(float, fovHalfLife, (0.25f), ("Fov Half Life", 0.01f, 2.0f)) // 足す量を寄せる半減期(秒)

		// HeadBob：走り出しに左右へ揺らす
		REFLECT_FIELD(float, runBobRearmSpeed, (1.0f), ("Run Bob Rearm Speed", 0.0f, 50.0f)) // この速さ以下で再発火できる(m/s)
		REFLECT_FIELD(float, runBobStepInterval, (0.3f), ("Run Bob Step Interval", 0.05f, 1.0f)) // 1歩の間隔(秒)
		REFLECT_FIELD(float, runBobDuration, (1.5f), ("Run Bob Duration", 0.1f, 5.0f))  // 消えるまでの時間(秒)
		REFLECT_FIELD(float, runBobSway, (0.15f), ("Run Bob Sway", 0.0f, 1.0f))  // 横ずれの最大(m)
		REFLECT_FIELD(float, runBobLift, (0.05f), ("Run Bob Lift", 0.0f, 1.0f))  // 上下の最大(m)。横の倍の速さで∞を描く
		REFLECT_FIELD(float, runBobRollDeg, (2.0f), ("Run Bob Roll Deg", 0.0f, 15.0f)) // 傾きの最大(度)

		// 演出ごとの状態
		REFLECT_FIELD(float, fovAdd, (0.0f), ("Fov Add", 0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // Fov：今足している量(度)
		REFLECT_FIELD(float, runBobTime, (0.0f), ("Run Bob Time", 0.0f, 0.0f, 0.01f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // HeadBob：発火からの経過(秒)
		REFLECT_FIELD(bool, runBobActive, (false), ("Run Bob Active", 0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // HeadBob：再生中
		REFLECT_FIELD(bool, runBobArmed, (true), ("Run Bob Armed", 0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // HeadBob：発火準備できている


		// 
		REFLECT_FIELD(DirectX::XMFLOAT3, eyeOffset, (0.0f, 0.0f, 0.0f), ("Eye Offset", 0.0f, 0.0f, 0.01f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // eye のずれ
		REFLECT_FIELD(DirectX::XMFLOAT3, targetOffset, (0.0f, 0.0f, 0.0f), ("Target Offset", 0.0f, 0.0f, 0.01f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // target のずれ
		REFLECT_FIELD(float, fovOffset, (0.0f), ("Fov Offset", 0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // FOV の増減(度)
		REFLECT_FIELD(float, roll, (0.0f), ("Roll", 0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 傾き(度。＋で右へ傾く)


		REFLECT_END()
};