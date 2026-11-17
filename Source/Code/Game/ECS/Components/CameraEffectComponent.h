#pragma once
#include <DirectXMath.h>
#include "Engine/ECS/Reflect.h"

// カメラの演出
struct CameraEffectComponent
{
	REFLECT_BEGIN(CameraEffectComponent, "Camera Effect")

		// Fov：速さで広げる
		REFLECT_FIELD(float, fovAddMax,   (15.0f), ("Fov Add Max",   0.0f, 60.0f)) // 速い時に足す FOV(度)
		REFLECT_FIELD(float, speedStart,   (5.0f), ("Speed Start",   0.0f, 50.0f)) // この速さから広げ始める(m/s)
		REFLECT_FIELD(float, speedFull,   (15.0f), ("Speed Full",    0.0f, 50.0f)) // この速さで fovAddMax になる(m/s)
		REFLECT_FIELD(float, fovHalfLife, (0.25f), ("Fov Half Life", 0.01f, 2.0f)) // 足す量を寄せる半減期(秒)

		// 演出ごとの状態
		REFLECT_FIELD(float, fovAdd, (0.0f), ("Fov Add", 0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // Fov：今足している量(度)

		// 
		REFLECT_FIELD(DirectX::XMFLOAT3, eyeOffset, (0.0f, 0.0f, 0.0f), ("Eye Offset", 0.0f, 0.0f, 0.01f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // eye のずれ
		REFLECT_FIELD(DirectX::XMFLOAT3, targetOffset, (0.0f, 0.0f, 0.0f), ("Target Offset", 0.0f, 0.0f, 0.01f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // target のずれ
		REFLECT_FIELD(float, fovOffset, (0.0f), ("Fov Offset", 0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // FOV の増減(度)
		REFLECT_FIELD(float, roll, (0.0f), ("Roll", 0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 傾き(度。＋で右へ傾く)


		REFLECT_END()
};