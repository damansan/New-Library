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
    // スティック
    ctrl.leftStickX = ds5InputState.leftStick.x / 127.0f;
    ctrl.leftStickY = ds5InputState.leftStick.y / 127.0f;
    ctrl.rightStickX = ds5InputState.rightStick.x / 127.0f;
    ctrl.rightStickY = ds5InputState.rightStick.y / 127.0f;

    // トリガー
    ctrl.leftTrigger = ds5InputState.leftTrigger / 255.0f;
    ctrl.rightTrigger = ds5InputState.rightTrigger / 255.0f;

    // ボタン
    auto btn = ds5InputState.buttonsAndDpad;
    if (btn & DS5W_ISTATE_BTX_CROSS)    ctrl.buttons |= ControllerButtons::Cross;
    if (btn & DS5W_ISTATE_BTX_CIRCLE)   ctrl.buttons |= ControllerButtons::Circle;
    if (btn & DS5W_ISTATE_BTX_SQUARE)   ctrl.buttons |= ControllerButtons::Square;
    if (btn & DS5W_ISTATE_BTX_TRIANGLE) ctrl.buttons |= ControllerButtons::Triangle;

    // ボタン
    auto btnA = ds5InputState.buttonsA;
    if (btnA & DS5W_ISTATE_BTN_A_LEFT_BUMPER)  ctrl.buttons |= ControllerButtons::L1;
    if (btnA & DS5W_ISTATE_BTN_A_RIGHT_BUMPER) ctrl.buttons |= ControllerButtons::R1;
    if (btnA & DS5W_ISTATE_BTN_A_LEFT_STICK)   ctrl.buttons |= ControllerButtons::L3;
    if (btnA & DS5W_ISTATE_BTN_A_RIGHT_STICK)  ctrl.buttons |= ControllerButtons::R3;
    if (btnA & DS5W_ISTATE_BTN_A_SELECT)       ctrl.buttons |= ControllerButtons::Share;
    if (btnA & DS5W_ISTATE_BTN_A_MENU)         ctrl.buttons |= ControllerButtons::Options;

    // L2/R2 デジタル判定
    if (ctrl.leftTrigger >= settings.leftTriggerThreshold)  ctrl.buttons |= ControllerButtons::L2;
    if (ctrl.rightTrigger >= settings.rightTriggerThreshold) ctrl.buttons |= ControllerButtons::R2;
}

// PS5 出力送信
void ControllerSystem::SendPS5Output(ControllerComponent& ctrl)
{
    DS5W::TriggerEffect newLeft = {};
    DS5W::TriggerEffect newRight = {};

    ApplyTriggerEffect(newLeft, ctrl.leftTriggerEffect);
    ApplyTriggerEffect(newRight, ctrl.rightTriggerEffect);

    bool changed =
        !IsSameTriggerEffect(newLeft, lastLeftEffect) ||
        !IsSameTriggerEffect(newRight, lastRightEffect);

    // 前回と同じ内容なら送信しない
    if (!changed)
    {
        return;
    }

    ds5OutputState = {};
    ds5OutputState.leftTriggerEffect = newLeft;
    ds5OutputState.rightTriggerEffect = newRight;

    DS5W::setDeviceOutputState(&ds5Context, &ds5OutputState);

    lastLeftEffect = newLeft;
    lastRightEffect = newRight;
}

// AdaptiveTrigger::TriggerEffectからDS5W::TriggerEffectへの変換
void ControllerSystem::ApplyTriggerEffect(DS5W::TriggerEffect& dst, const AdaptiveTrigger::TriggerEffect& src)
{
    // ゾーンを検出して、最初のゾーンの「開始位置・強さ」をハードウェアにそのまま渡す
    auto zones = AdaptiveTrigger::DetectZones(src.forcePoints);

    if (zones.empty())
    {
        dst.effectType = DS5W::TriggerEffectType::NoResitance;
        return;
    }

    const auto& zone = zones.front();
    float startPos = zone.points.front().position;
    float force = zone.points.front().value;

    dst.effectType = DS5W::TriggerEffectType::ContinuousResitance;
    dst.Continuous.startPosition = static_cast<unsigned char>(startPos * 255.0f);
    dst.Continuous.force = static_cast<unsigned char>(force * 255.0f);
}

// 2つのTriggerEffectが同じ内容かどうか比較する
bool ControllerSystem::IsSameTriggerEffect(const DS5W::TriggerEffect& a, const DS5W::TriggerEffect& b)
{
    if (a.effectType != b.effectType)
    {
        return false;
    }
    if (a.effectType == DS5W::TriggerEffectType::ContinuousResitance)
    {
        return a.Continuous.startPosition == b.Continuous.startPosition &&
            a.Continuous.force == b.Continuous.force;
    }
    // NoResitanceなど、それ以外は種類が同じなら同じとみなす
    return true;
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

