#include "ControllerSystem.h"
#include <cmath>
#include <algorithm>

// コンストラクタ
ControllerSystem::ControllerSystem()
{
    TryConnectPS5();
}

// デストラクタ
ControllerSystem::~ControllerSystem()
{
    if (ds5Connected)
    {
        // 終了時にトリガー/振動をリセットしてから切断
        DS5W::DS5OutputState reset = {};
        DS5W::setDeviceOutputState(&ds5Context, &reset);
        DS5W::freeDeviceContext(&ds5Context);
        ds5Connected = false;
    }
}

// 更新
void ControllerSystem::Update(entt::registry& registry, float elapsedTime)
{
    // Xbox 接続確認
    xboxConnected = XInputGetState(0, &xboxState) == ERROR_SUCCESS;

    // PS5 接続確認 / 再接続
    if (ds5Connected)
    {
        DS5W_ReturnValue rv = DS5W::getDeviceInputState(&ds5Context, &ds5InputState);
        if (DS5W_FAILED(rv))
        {
            DS5W::freeDeviceContext(&ds5Context);
            ds5Connected = false;
        }
    }
    else
    {
        ds5ReconnectTimer += elapsedTime;
        if (ds5ReconnectTimer >= ds5ReconnectInterval)
        {
            ds5ReconnectTimer = 0.0f;
            TryConnectPS5();
        }
    }

    // ControllerComponent を持つ Entity を更新
    auto view = registry.view<ControllerComponent>();
    view.each([&](ControllerComponent& ctrl)
        {
            // リセット
            ctrl.leftStickX = 0.0f;
            ctrl.leftStickY = 0.0f;
            ctrl.rightStickX = 0.0f;
            ctrl.rightStickY = 0.0f;
            ctrl.leftTrigger = 0.0f;
            ctrl.rightTrigger = 0.0f;
            ctrl.buttons = 0;
            ctrl.connected = false;
            ctrl.type = ControllerType::None;

            // PS5 優先、なければ Xbox
            if (ds5Connected)
            {
                ctrl.type = ControllerType::PS5;
                ctrl.connected = true;
                UpdatePS5Input(ctrl);
                SendPS5Output(ctrl);
            }
            else if (xboxConnected)
            {
                ctrl.type = ControllerType::Xbox;
                ctrl.connected = true;
                UpdateXboxInput(ctrl);
            }
        });
}

// Xbox 入力
void ControllerSystem::UpdateXboxInput(ControllerComponent& ctrl)
{
    const auto& gp = xboxState.Gamepad;

    // スティック（-32768～32767 → -1.0～1.0）
    ctrl.leftStickX = gp.sThumbLX / 32767.0f;
    ctrl.leftStickY = gp.sThumbLY / 32767.0f;
    ctrl.rightStickX = gp.sThumbRX / 32767.0f;
    ctrl.rightStickY = gp.sThumbRY / 32767.0f;

    // トリガー（0～255 → 0.0～1.0）
    ctrl.leftTrigger = gp.bLeftTrigger / 255.0f;
    ctrl.rightTrigger = gp.bRightTrigger / 255.0f;

    // ボタン
    auto btn = gp.wButtons;
    if (btn & XINPUT_GAMEPAD_A)              ctrl.buttons |= ControllerButtons::Cross;
    if (btn & XINPUT_GAMEPAD_B)              ctrl.buttons |= ControllerButtons::Circle;
    if (btn & XINPUT_GAMEPAD_X)              ctrl.buttons |= ControllerButtons::Square;
    if (btn & XINPUT_GAMEPAD_Y)              ctrl.buttons |= ControllerButtons::Triangle;
    if (btn & XINPUT_GAMEPAD_LEFT_SHOULDER)  ctrl.buttons |= ControllerButtons::L1;
    if (btn & XINPUT_GAMEPAD_RIGHT_SHOULDER) ctrl.buttons |= ControllerButtons::R1;
    if (btn & XINPUT_GAMEPAD_LEFT_THUMB)     ctrl.buttons |= ControllerButtons::L3;
    if (btn & XINPUT_GAMEPAD_RIGHT_THUMB)    ctrl.buttons |= ControllerButtons::R3;
    if (btn & XINPUT_GAMEPAD_START)          ctrl.buttons |= ControllerButtons::Options;
    if (btn & XINPUT_GAMEPAD_BACK)           ctrl.buttons |= ControllerButtons::Share;

    // L2/R2 デジタル判定（settings の閾値を使う）
    if (ctrl.leftTrigger >= settings.leftTriggerThreshold)  ctrl.buttons |= ControllerButtons::L2;
    if (ctrl.rightTrigger >= settings.rightTriggerThreshold) ctrl.buttons |= ControllerButtons::R2;
}

// PS5 入力
void ControllerSystem::UpdatePS5Input(ControllerComponent& ctrl)
{
    // スティック（-128～127 → -1.0～1.0）
    ctrl.leftStickX = ds5InputState.leftStick.x / 127.0f;
    ctrl.leftStickY = ds5InputState.leftStick.y / 127.0f;
    ctrl.rightStickX = ds5InputState.rightStick.x / 127.0f;
    ctrl.rightStickY = ds5InputState.rightStick.y / 127.0f;

    // トリガー（0～255 → 0.0～1.0）
    ctrl.leftTrigger = ds5InputState.leftTrigger / 255.0f;
    ctrl.rightTrigger = ds5InputState.rightTrigger / 255.0f;

    // ボタン（buttonsAndDpad）
    auto btn = ds5InputState.buttonsAndDpad;
    if (btn & DS5W_ISTATE_BTX_CROSS)    ctrl.buttons |= ControllerButtons::Cross;
    if (btn & DS5W_ISTATE_BTX_CIRCLE)   ctrl.buttons |= ControllerButtons::Circle;
    if (btn & DS5W_ISTATE_BTX_SQUARE)   ctrl.buttons |= ControllerButtons::Square;
    if (btn & DS5W_ISTATE_BTX_TRIANGLE) ctrl.buttons |= ControllerButtons::Triangle;

    // ボタン（buttonsA）
    auto btnA = ds5InputState.buttonsA;
    if (btnA & DS5W_ISTATE_BTN_A_LEFT_BUMPER)  ctrl.buttons |= ControllerButtons::L1;
    if (btnA & DS5W_ISTATE_BTN_A_RIGHT_BUMPER) ctrl.buttons |= ControllerButtons::R1;
    if (btnA & DS5W_ISTATE_BTN_A_LEFT_STICK)   ctrl.buttons |= ControllerButtons::L3;
    if (btnA & DS5W_ISTATE_BTN_A_RIGHT_STICK)  ctrl.buttons |= ControllerButtons::R3;
    if (btnA & DS5W_ISTATE_BTN_A_SELECT)       ctrl.buttons |= ControllerButtons::Share;
    if (btnA & DS5W_ISTATE_BTN_A_MENU)         ctrl.buttons |= ControllerButtons::Options;

    // L2/R2 デジタル判定（settings の閾値を使う）
    if (ctrl.leftTrigger >= settings.leftTriggerThreshold)  ctrl.buttons |= ControllerButtons::L2;
    if (ctrl.rightTrigger >= settings.rightTriggerThreshold) ctrl.buttons |= ControllerButtons::R2;
}

// PS5 出力送信
void ControllerSystem::SendPS5Output(ControllerComponent& ctrl)
{
    ds5OutputState = {};

    // 振動
    ds5OutputState.leftRumble = ctrl.vibration.leftMotor;
    ds5OutputState.rightRumble = ctrl.vibration.rightMotor;

    // アダプティブトリガー
    ApplyTriggerEffect(ds5OutputState.leftTriggerEffect, settings.leftTriggerEffect);
    ApplyTriggerEffect(ds5OutputState.rightTriggerEffect, settings.rightTriggerEffect);

    DS5W::setDeviceOutputState(&ds5Context, &ds5OutputState);
}

// TriggerEffect変換
void ControllerSystem::ApplyTriggerEffect(DS5W::TriggerEffect& dst, const TriggerEffect& src)
{
    switch (src.type)
    {
    case TriggerEffectType::Off:
        dst.effectType = DS5W::TriggerEffectType::NoResitance;
        break;

    case TriggerEffectType::Rigid:
        dst.effectType = DS5W::TriggerEffectType::ContinuousResitance;
        dst.Continuous.startPosition = src.start;
        dst.Continuous.force = src.force;
        break;

    case TriggerEffectType::Pulse:
        dst.effectType = DS5W::TriggerEffectType::SectionResitance;
        dst.Section.startPosition = src.start;
        dst.Section.endPosition = src.end;
        break;

    case TriggerEffectType::Slope:
        dst.effectType = DS5W::TriggerEffectType::ContinuousResitance;
        dst.Continuous.startPosition = src.start;
        dst.Continuous.force = static_cast<uint8_t>(
            src.force * (1.0f - src.start / 255.0f));
        break;

    default:
        dst.effectType = DS5W::TriggerEffectType::NoResitance;
        break;
    }
}

// PS5 接続試行
void ControllerSystem::TryConnectPS5()
{
    DS5W::DeviceEnumInfo enumInfo[4];
    unsigned int deviceCount = 0;

    if (DS5W_SUCCESS(DS5W::enumDevices(enumInfo, 4, &deviceCount, true)))
    {
        if (deviceCount > 0)
        {
            if (DS5W_SUCCESS(DS5W::initDeviceContext(&enumInfo[0], &ds5Context)))
            {
                ds5Connected = true;
            }
        }
    }
}

