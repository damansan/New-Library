#pragma once

enum class PlayerDodgeState
{
	None,      
	Deciding,   // 猶予中
	Dodge,      // 回避

	FrontDodge, // 前回避
	BackDodge,  // 後回避
	LeftDodge,  // 左回避
	RightDodge, // 右回避
};

struct PlayerDodgeComponent
{
	// 設定値
	float triggerThreshold = 0.3f;        // これ以上で「押した」とみなす
	float triggerReleaseThreshold = 0.1f; // これ以下で「離した」とみなす
	float directionBufferTime = 0.12f;    // スティック入力を待つ猶予時間

	// 状態
	PlayerDodgeState state = PlayerDodgeState::None;

	// 状態フラグ
	bool isDodging = false; // 回避中かどうか

	bool isNoneInput = false;  // 入力なし状態かどうか
	bool isFrontInput = false; // 前入力中かどうか
	bool isBackInput = false;  // 後入力中かどうか
	bool isLeftInput = false;  // 左入力中かどうか
	bool isRightInput = false; // 右入力中かどうか


	// 内部実装用
	bool wasTriggerHeld = false; // 前フレームでトリガーが押されていたかどうか
	float bufferTimer = 0.0f;    // 猶予経過時間
};