#pragma once

enum class PlayerSprintSubState
{
	None,
	Sprint, // ダッシュ
};

struct PlayerSprintComponent
{
	// 設定値
	float sprintSpeed = 15.0f; // ダッシュ速度

	float turnSpeed = 15.0f;    // 1秒あたりの最大旋回速度

	// 状態
	PlayerSprintSubState state = PlayerSprintSubState::None;

	// 状態フラグ
	bool isSprinting = false;
};