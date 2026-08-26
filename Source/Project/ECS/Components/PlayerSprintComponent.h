#pragma once

enum class PlayerSprintState
{
	None,
	Sprinting, // ダッシュ
};

struct PlayerSprintComponent
{
	// 設定値
	float sprintSpeed = 15.0f; // ダッシュ速度

	// 状態
	PlayerSprintState state = PlayerSprintState::None;

	// 状態フラグ
	bool isSprinting = false;
};