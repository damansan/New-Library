#pragma once
#include "Engine/ECS/Reflect.h"

struct CameraEffectComponent
{
	REFLECT_BEGIN(CameraEffectComponent, "Camera Effect")

		// 速さで FOV を広げる
		REFLECT_FIELD(float, baseFov,     (40.0f), ("Base Fov", 10.0f, 120.0f))  // 基本 FOV
		REFLECT_FIELD(float, maxFov,      (80.0f), ("Max Fov", 10.0f, 120.0f))   // 速い時の FOV
		REFLECT_FIELD(float, speedStart,   (5.0f), ("Speed Start", 0.0f, 50.0f)) // この速さから広げ始める
		REFLECT_FIELD(float, speedFull,   (15.0f), ("Speed Full", 0.0f, 50.0f))  // この速さで maxFov になる
		REFLECT_FIELD(float, fovHalfLife, (0.25f), ("Fov Half Life", 0.01f, 2.0f)) // 目標の FOV へ寄せる半減期

		// 状態
		REFLECT_FIELD(float, targetFov,  (0.0f), ("Target Fov",  0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 今の速さから決めた FOV
		REFLECT_FIELD(float, currentFov, (0.0f), ("Current Fov", 0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 実際に使っている FOV

		REFLECT_END()
};