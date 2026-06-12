#pragma once
#include <cstdint>

// コントローラーの種類
enum class ControllerType
{
    None,
    Xbox,
    PS5,
};

// アダプティブトリガーのエフェクト　
enum class TriggerEffectType : uint8_t
{
    Off = 0,    // 無効
    Rigid = 1,  // 一定抵抗
    Pulse = 2,  // 振動
    Slope = 3,  // 傾斜（引くほど重くなる）
};

// トリガー1本分の出力設定
struct TriggerEffect
{
    TriggerEffectType type = TriggerEffectType::Off;
    uint8_t           start = 0;    // 効果開始位置 (0～255)
    uint8_t           end = 255;    // 効果終了位置 (0～255)
    uint8_t           force = 128;  // 抵抗の強さ   (0～255)
    uint8_t           freq = 0;     // 振動周波数   (Pulseのとき使用)
};

// 振動出力設定
struct VibrationOutput
{
    uint8_t leftMotor = 0;  // 左モーター (0～255)
    uint8_t rightMotor = 0; // 右モーター (0～255)
};

// コントローラーの生の入力値を保持するコンポーネント
struct ControllerComponent
{
    // 接続情報
    ControllerType type = ControllerType::None;
    bool           connected = false;

    // スティック（正規化済み -1.0～1.0、デッドゾーン未適用）
    float leftStickX = 0.0f;
    float leftStickY = 0.0f;
    float rightStickX = 0.0f;
    float rightStickY = 0.0f;

    // トリガー（正規化済み 0.0～1.0）
    float leftTrigger = 0.0f;
    float rightTrigger = 0.0f;

    // ボタン（ビットフラグ）
    uint32_t buttons = 0;

    // 振動出力（ControllerSystem が毎フレーム送信）
    VibrationOutput vibration = {};

    // アダプティブトリガー出力（PS5のみ）
    TriggerEffect leftTriggerEffect = {};
    TriggerEffect rightTriggerEffect = {};

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