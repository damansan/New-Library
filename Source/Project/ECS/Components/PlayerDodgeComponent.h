#pragma once

enum class PlayerDodgeState
{
	None,      
	Dodging,   // 回避
};

struct PlayerDodgeComponent
{
	// 設定値
	float triggerThreshold = 0.3f;        // これ以上で「押した」とみなす
	float triggerReleaseThreshold = 0.1f; // これ以下で「離した」とみなす

	// 状態
	PlayerDodgeState state = PlayerDodgeState::None;

	// 状態フラグ
	bool isDodging = false;


	// 内部実装用
	bool wasTriggerHeld = false; // 前フレームでトリガーが押されていたかどうか
};