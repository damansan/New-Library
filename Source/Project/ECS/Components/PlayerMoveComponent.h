#pragma once

enum class PlayerMovementState
{
	None, 
	Walking,   // 歩き
	Jogging,   // 小走り
};

struct PlayerMoveComponent
{
	// 設定値
	float walkSpeed    = 2.0f; // 歩き速度
	float jogSpeed     = 4.0f; // 小走り速度
	float sprintSpeed  = 6.0f; // ダッシュ速度
	float jogThreshold = 0.8f; // スティックの倒し具合が、この値を超えたら小走りとみなす
	
	// 状態
	PlayerMovementState state = PlayerMovementState::None;

	// 状態フラグ
	bool isMoving = false;  // 現在、移動中かどうか
	bool isWalking = false; // 現在、歩き判定かどうか
	bool isJogging = false; // 現在、小走り判定かどうか
};