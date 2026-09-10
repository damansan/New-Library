#pragma once

enum class PlayerIdleSubState
{
	None,   
	Idle,   // 待機中
};

struct PlayerIdleComponent
{
	// 設定値


	// 状態
	PlayerIdleSubState idleSub = PlayerIdleSubState::None;

	// 状態フラグ
	bool isIdling = false;
};