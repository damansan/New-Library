#pragma once

enum class PlayerMovementState
{
	Idle,      // 待機
	Walking,   // 歩き
	Jogging,   // 小走り
	Dodging,   // 回避
	Sprinting, // ダッシュ
};

struct PlayerMoveComponent
{
	// 移動速度など
	float walkSpeed = 2.0f; // 歩き速度
	float jogSpeed = 4.0f; // 小走り速度
	float sprintSpeed = 6.0f; // ダッシュ速度
	float jogThreshold = 0.8f; // スティックの倒し具合が、この値を超えたら小走りとみなす
	
	// Playerの移動状態
	PlayerMovementState state = PlayerMovementState::Idle;

	// 移動状態のフラグ
	bool isMoving = false;    // 現在、移動中かどうか
	bool isWalking = false;   // 現在、歩き判定かどうか
	bool isJogging = false;   // 現在、小走り判定かどうか
	bool isDodging = false;   // 現在、回避中かどうか
	bool isSprinting = false; // 現在、ダッシュ中かどうか
};