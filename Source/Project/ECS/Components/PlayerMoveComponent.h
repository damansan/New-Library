#pragma once
#include "Engine/ECS/Reflect.h"

enum class PlayerMoveSubState
{
	None,
	Walking, // 歩き
	Jogging, // 小走り
};

struct PlayerMoveComponent
{
	// 状態
	PlayerMoveSubState moveSub = PlayerMoveSubState::None;

	REFLECT_BEGIN(PlayerMoveComponent, "Player Move")

		// 設定値
		REFLECT_FIELD(float, walkSpeed,    (2.0f),  ("Walk Speed",    0.0f, 10.0f))
		REFLECT_FIELD(float, jogSpeed,     (4.0f),  ("Jog Speed",     0.0f, 10.0f))
		REFLECT_FIELD(float, jogThreshold, (0.9f),  ("Jog Threshold", 0.0f, 1.0f))
		REFLECT_FIELD(float, turnSpeed,    (10.0f), ("Turn Speed",    0.0f, 30.0f))

		// 状態フラグ
		REFLECT_FIELD(bool, isWalking, (false), ("Is Walking"))
		REFLECT_FIELD(bool, isJogging, (false), ("Is Jogging"))
		REFLECT_FIELD(bool, isMoving,  (false), ("Is Moving"))

	REFLECT_END()

		// 内部実装用
		float inputLossGraceTime = 0.1f; // 入力が一瞬無くなっても、猶予として維持する時間
		float inputLossTimer = 0.0f;     // 内部実装用、入力が無くなってからの経過時間
};