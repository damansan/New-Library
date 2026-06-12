#pragma once
#include <cstdint>
#include "ECS/Components/ControllerComponent.h"

// コントローラーの設定値（JSON保存）
struct ControllerSettings
{
	// スティック デッドゾーン(0.0～1.0)
	float leftStickDeadzone = 0.2f;
	float rightStickDeadzone = 0.2f;

	// トリガー デッドゾーン(0.0～1.0)
	float leftTriggerDeadzone = 0.05f;
	float rightTriggerDeadzone = 0.05f;

	// L2/R2 デジタル判定閾値 (0.0～1.0)
	// どれだけ引いたら「押した」と判定するか
	float leftTriggerThreshold = 0.5f;
	float rightTriggerThreshold = 0.5f;

	// アダプティブトリガー設定（PS5のみ）
	TriggerEffect leftTriggerEffect = {};
	TriggerEffect rightTriggerEffect = {};

	// 振動設定 (0～255)
	uint8_t leftMotor = 0;
	uint8_t rightMotor = 0;
};