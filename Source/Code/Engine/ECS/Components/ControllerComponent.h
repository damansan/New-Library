#pragma once
#include <cstdint>
#include "Engine/ECS/Components/AdaptiveTrigger.h"

// コントローラーの種類
enum class ControllerType
{
    None,
    Xbox,
    PS5,
};

// トリガーのヒステリシス状態
struct TriggerHysteresisState
{
    bool wasHeld = false;
};

// コントローラーの生の入力値を保持するコンポーネント
struct ControllerComponent
{
    // 接続情報
    ControllerType type = ControllerType::None;
    bool           connected = false;

    // スティック
    float leftStickX = 0.0f;
    float leftStickY = 0.0f;
    float rightStickX = 0.0f;
    float rightStickY = 0.0f;

    // トリガー
    float leftTrigger = 0.0f;
    float rightTrigger = 0.0f;

    // ボタン
    uint32_t buttons = 0;

    // アダプティブトリガー出力
    AdaptiveTrigger::TriggerEffect leftTriggerEffect = {};
    AdaptiveTrigger::TriggerEffect rightTriggerEffect = {};

	// トリガーのヒステリシス状態
    TriggerHysteresisState leftTriggerHysteresis;
    TriggerHysteresisState rightTriggerHysteresis;

    // ボタン判定ヘルパー
    bool IsPressed(uint32_t mask) const { return (buttons & mask) != 0; }
};

// ボタンのビットマスク定数
namespace ControllerButtons
{
    constexpr uint32_t Cross = 1 << 0;     // PS5:×  Xbox:A
    constexpr uint32_t Circle = 1 << 1;    // PS5:○  Xbox:B
    constexpr uint32_t Square = 1 << 2;    // PS5:□  Xbox:X
    constexpr uint32_t Triangle = 1 << 3;  // PS5:△  Xbox:Y
    constexpr uint32_t L1 = 1 << 4;
    constexpr uint32_t R1 = 1 << 5;
    constexpr uint32_t L2 = 1 << 6;  // デジタル判定用
    constexpr uint32_t R2 = 1 << 7;  // デジタル判定用
    constexpr uint32_t L3 = 1 << 8;  // スティック押し込み
    constexpr uint32_t R3 = 1 << 9;  // スティック押し込み
    constexpr uint32_t Options = 1 << 14; // PS5:Options  Xbox:Menu
    constexpr uint32_t Share = 1 << 15;   // PS5:Create   Xbox:View
}