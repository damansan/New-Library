#pragma once
#include "Engine/ECS/Reflect.h"

enum class PlayerSprintSubState
{
	None,
	Sprint, // ダッシュ
};

struct PlayerSprintComponent
{
	// 状態
	PlayerSprintSubState state = PlayerSprintSubState::None;
	
	REFLECT_BEGIN(PlayerSprintComponent, "Player Sprint")

		// 設定値
		REFLECT_FIELD(float, sprintSpeed, (15.0f), ("Sprint Speed", 0.0f, 20.0f))
		REFLECT_FIELD(float, turnSpeed,   (15.0f), ("Turn Speed",   0.0f, 30.0f))

		// 状態フラグ
		REFLECT_FIELD(bool, isSprinting, (false), ("Is Sprinting", 0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient))

	REFLECT_END()
};