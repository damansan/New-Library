#pragma once
#include "Engine/ECS/Reflect.h"

// スティックの状態
struct StickState
{
    REFLECT_BEGIN(StickState, "Stick State")
    
        REFLECT_FIELD(float, x,        (0.0f),  ("X",         0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient))
        REFLECT_FIELD(float, y,        (0.0f),  ("Y",         0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient))
        REFLECT_FIELD(bool,  hasInput, (false), ("Has Input", 0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 入力があったかどうか

    REFLECT_END()
};

// ボタンの状態
struct ButtonState
{
    REFLECT_BEGIN(ButtonState, "Button State")
        
        REFLECT_FIELD(bool, pressed,  (false), ("Pressed",  0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 入力があったかどうか
        REFLECT_FIELD(bool, held,     (false), ("Held",     0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 押しっぱなし中
        REFLECT_FIELD(bool, released, (false), ("Released", 0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 今フレーム離された
        
    REFLECT_END()

        // 内部用：前フレームの押下状態
        bool wasHeld = false;
};

// トリガーの状態
struct TriggerState
{
    REFLECT_BEGIN(TriggerState, "Trigger State")

        REFLECT_FIELD(float, value,    (0.0f),  ("Value",    0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 入力値(0.0～1.0)
        REFLECT_FIELD(bool,  pressed,  (false), ("Pressed",  0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 今フレーム新しく押された
        REFLECT_FIELD(bool,  held,     (false), ("Held",     0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 押しっぱなし中
        REFLECT_FIELD(bool,  released, (false), ("Released", 0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 今フレーム離された
        
    REFLECT_END()

        // 内部用：前フレームの押下状態
        bool wasHeld = false;
};

struct GameInputComponent
{
    REFLECT_BEGIN(GameInputComponent, "Input")
    
        // スティック
        REFLECT_FIELD(StickState, moveStick,   (), ("Move Stick"))
        REFLECT_FIELD(StickState, cameraStick, (), ("Camera Stick"))

        // トリガー
        REFLECT_FIELD(TriggerState, dodgeTrigger,  (), ("Dodge Trigger"))   // R2
        REFLECT_FIELD(TriggerState, lockOnTrigger, (), ("Lock On Trigger")) // L2

        // ボタン
        REFLECT_FIELD(ButtonState, jump, (), ("Jump"))
        REFLECT_FIELD(bool, attack, (false), ("Attack", 0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient))
        
    REFLECT_END()
};