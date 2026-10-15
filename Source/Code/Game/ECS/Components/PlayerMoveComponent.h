#pragma once
#include "Engine/ECS/Reflect.h"

// Move ステートの調整値
struct PlayerMoveComponent
{
	REFLECT_BEGIN(PlayerMoveComponent, "Player Move")

		REFLECT_FIELD(float, walkSpeed, (1.7f), ("Walk Speed", 0.0f, 10.0f)) // Walk_Loop の想定速度
		REFLECT_FIELD(float, runSpeed, (4.2f), ("Run Speed", 0.0f, 10.0f)) // Run_Loop の想定速度
		REFLECT_FIELD(float, accelHalfLife, (0.05f), ("Accel Half Life", 0.0f, 1.0f))  // 加速時の半減期(秒)
		REFLECT_FIELD(float, decelHalfLife, (0.08f), ("Decel Half Life", 0.0f, 1.0f))  // 減速時の半減期(秒)。旋回で目標速度が下がった時など
		REFLECT_FIELD(float, turnSpeed, (10.0f), ("Turn Speed", 0.0f, 30.0f)) // 旋回速度(rad/s)

		REFLECT_END()
};