#pragma once
#include "Engine/ECS/Reflect.h"

enum class PlayerDodgeSubState
{
	None,
	Deciding, // 方向決定の猶予中
	InPlace,  // その場回避
	Front,    // 前方回避
	Back,     // 後方回避
	Left,     // 左回避
	Right,    // 右回避
};

struct PlayerDodgeComponent
{
	// 状態
	PlayerDodgeSubState dodgeSub = PlayerDodgeSubState::None;

	REFLECT_BEGIN(PlayerDodgeComponent, "Player Dodge")

		// 設定値
		REFLECT_FIELD(float, directionBufferTime, (0.12f), ("Direction Buffer Time", 0.0f, 1.0f))

		// 状態フラグ
		REFLECT_FIELD(bool, isDodging,    (false), ("Is Dodging",     0.0f, 0.0f, 0.1f, "", true))
		REFLECT_FIELD(bool, isNoneInput,  (false), ("Is None Input",  0.0f, 0.0f, 0.1f, "", true))
		REFLECT_FIELD(bool, isFrontInput, (false), ("Is Front Input", 0.0f, 0.0f, 0.1f, "", true))
		REFLECT_FIELD(bool, isBackInput,  (false), ("Is Back Input",  0.0f, 0.0f, 0.1f, "", true))
		REFLECT_FIELD(bool, isLeftInput,  (false), ("Is Left Input",  0.0f, 0.0f, 0.1f, "", true))
		REFLECT_FIELD(bool, isRightInput, (false), ("Is Right Input", 0.0f, 0.0f, 0.1f, "", true))

	REFLECT_END()

	// 内部実装用
	float bufferTimer = 0.0f; // 猶予経過時間
};