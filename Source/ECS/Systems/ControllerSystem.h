#pragma once
#pragma comment(lib, "Xinput.lib")

#include <windows.h>
#include <Xinput.h>
#include <DualSenseWindows/IO.h>

#include "entt.hpp"
#include "ECS/Components/ControllerComponent.h"
#include "ECS/Components/ControllerSettings.h"

class ControllerSystem
{
public:
    ControllerSystem();
    ~ControllerSystem();

    ControllerSystem(const ControllerSystem&) = delete;
    ControllerSystem& operator=(const ControllerSystem&) = delete;
    ControllerSystem(ControllerSystem&&) = delete;
    ControllerSystem& operator=(ControllerSystem&&) = delete;

    // 更新
    void Update(entt::registry& registry, float elapsedTime);

    // 設定への参照（ControllerToolWindow から読み書きする）
    ControllerSettings& GetSettings() { return settings; }
    const ControllerSettings& GetSettings() const { return settings; }

private:
    // Xbox 入力をControllerComponentに書く
    void UpdateXboxInput(ControllerComponent& ctrl);

    // PS5 入力をControllerComponentに書く
    void UpdatePS5Input(ControllerComponent& ctrl);

    // PS5 出力送信（振動/アダプティブトリガー）
    void SendPS5Output(ControllerComponent& ctrl);

    // TriggerEffect変換
    void ApplyTriggerEffect(DS5W::TriggerEffect& dst, const TriggerEffect& src, float currentValue);

    // PS5 接続試行
    void TryConnectPS5();

    // Xbox
    bool         xboxConnected = false;
    XINPUT_STATE xboxState = {};

    // PS5
    DS5W::DeviceContext  ds5Context = {};
    DS5W::DS5InputState  ds5InputState = {};
    DS5W::DS5OutputState ds5OutputState = {};
    bool                 ds5Connected = false;
    float                ds5ReconnectTimer = 0.0f;
    static constexpr float ds5ReconnectInterval = 3.0f;

    // 設定値
    ControllerSettings settings;
};