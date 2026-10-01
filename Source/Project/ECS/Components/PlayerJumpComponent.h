#pragma once
#include "Engine/ECS/Reflect.h"

enum class PlayerJumpSubState
{
	None,
	Start, // 跳躍
	Loop,  // 空中
	End,   // 着地
};

struct PlayerJumpComponent
{
	// 状態
	PlayerJumpSubState jumpSub = PlayerJumpSubState::None;

	REFLECT_BEGIN(PlayerJumpComponent, "Player Jump")

		// 設定値
		REFLECT_FIELD(float, jumpPower, (8.0f), ("Jump Power", 0.0f, 50.0f))

		// 状態フラグ
		REFLECT_FIELD(bool, isJumping, (false), ("Is Jumping", 0.0f, 0.0f, 0.1f, "", true))

	REFLECT_END()
};