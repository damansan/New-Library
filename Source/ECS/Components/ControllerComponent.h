#pragma once
#include <cstdint>
#include <vector>

// コントローラーの種類
enum class ControllerType
{
    None,
    Xbox,
    PS5,
};

// カーブ上の1点（区間内の位置と、そこでの強さ）
struct CurvePoint
{
    float position = 0.0f; // 区間内の位置 (0.0〜1.0)
    float value = 0.0f;    // 強さ         (0.0〜1.0)
};

// トリガー1本分の出力設定
struct TriggerEffect
{
    uint8_t start = 0;    // 効果区間の開始位置 (0〜255)
    uint8_t end = 255;    // 効果区間の終了位置 (0〜255)

    // 区間内での抵抗力の変化カーブ（デフォルトは中間の高さで平ら）
    std::vector<CurvePoint> forcePoints =
    {
        { 0.0f, 0.5f },
        { 1.0f, 0.5f },
    };

    // 区間内での振動の強さの変化カーブ（デフォルトは振動なし）
    std::vector<CurvePoint> vibrationPoints =
    {
        { 0.0f, 0.0f },
        { 1.0f, 0.0f },
    };
};

// カーブ上の指定positionにおける値を線形補間で求める
inline float SampleCurve(const std::vector<CurvePoint>& points, float position)
{
    if (points.empty())
    {
        return 0.0f;
    }
    if (points.size() == 1 || position <= points.front().position)
    {
        return points.front().value;
    }
    if (position >= points.back().position)
    {
        return points.back().value;
    }

    for (size_t i = 0; i + 1 < points.size(); ++i)
    {
        const CurvePoint& a = points[i];
        const CurvePoint& b = points[i + 1];
        if (position >= a.position && position <= b.position)
        {
            float t = (position - a.position) / (b.position - a.position);
            return a.value + (b.value - a.value) * t;
        }
    }
    return points.back().value;
}

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