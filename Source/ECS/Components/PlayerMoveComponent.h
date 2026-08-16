#pragma once

struct PlayerMoveComponent
{
	float walkSpeed = 2.0f; // 歩き速度
	float jogSpeed = 4.0f;  // 小走り速度

	float jogThreshold = 0.8f; // スティックの倒し具合が、この値を超えたら小走りとみなす

	bool isWalking = false; // 現在、歩き判定かどうか
	bool isJogging = false; // 現在、小走り判定かどうか

	bool isMoving = false;  // 現在、移動中かどうか
};