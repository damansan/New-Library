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
	float jogThreshold = 0.9f; // スティックの倒し具合が、この値を超えたら小走りとみなす
	
	float turnSpeed = 10.0f; // １秒当たりの最大旋回速度

	// 状態
	PlayerMovementState state = PlayerMovementState::None;

	// 状態フラグ
	bool isMoving = false;  // 現在、移動中かどうか
	bool isWalking = false; // 現在、歩き判定かどうか
	bool isJogging = false; // 現在、小走り判定かどうか


	// 内部実装用
	float inputLossGraceTime = 0.1f; // 入力が一瞬無くなっても、猶予として維持する時間
	float inputLossTimer = 0.0f;     // 内部実装用、入力が無くなってからの経過時間
	bool wasControlLocked = false;   // 前フレーム、xzControlLockedがtrueだったか
};