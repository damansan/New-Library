#pragma once

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
	// 設定値
	float directionBufferTime = 0.12f; // スティック入力を待つ猶予時間

	// 状態
	PlayerDodgeSubState dodgeSub = PlayerDodgeSubState::None;

	// 状態フラグ
	bool isDodging = false; // 回避中かどうか

	bool isNoneInput = false;  // 入力なし状態かどうか
	bool isFrontInput = false; // 前入力中かどうか
	bool isBackInput = false;  // 後入力中かどうか
	bool isLeftInput = false;  // 左入力中かどうか
	bool isRightInput = false; // 右入力中かどうか

	// 内部実装用
	float bufferTimer = 0.0f; // 猶予経過時間
};