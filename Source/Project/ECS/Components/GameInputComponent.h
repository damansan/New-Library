#pragma once

// スティックの状態
struct StickState
{
    float x = 0.0f;
    float y = 0.0f;

	// 入力があったかどうか
    bool hasInput = false;
};

// ボタンの状態
struct ButtonState
{
    bool pressed = false;  // 今フレーム新しく押された
    bool held = false;     // 押しっぱなし中
    bool released = false; // 今フレーム離された

    // 内部用：前フレームの押下状態
    bool wasHeld = false;
};

// トリガーの状態
struct TriggerState
{
    float value = 0.0f; // 入力値(0.0～1.0)

    bool pressed = false;  // 今フレーム新しく押された
    bool held = false;     // 押しっぱなし中
    bool released = false; // 今フレーム離された

    // 内部用：前フレームの押下状態
    bool wasHeld = false;  
};

struct GameInputComponent
{
    // スティック
    StickState moveStick;
    StickState cameraStick;

    // トリガー
    TriggerState dodgeTrigger;

    // ボタン
    bool jump = false;
    bool attack = false;
};
