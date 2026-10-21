#pragma once
#include "Engine/ECS/Reflect.h"

#include "Engine/ECS/Reflect.h"

// Idle ステートの調整値
struct PlayerIdleComponent
{
	REFLECT_BEGIN(PlayerIdleComponent, "Player Idle")

		REFLECT_FIELD(float, stopHalfLife, (0.06f), ("Stop Half Life", 0.0f, 1.0f)) // 停止時の半減期

	REFLECT_END()
};