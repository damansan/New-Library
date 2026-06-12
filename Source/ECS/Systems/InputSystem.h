#pragma once
#include "entt.hpp"
#include "ECS/Components/InputComponent.h"
#include "ECS/Components/ControllerComponent.h"
#include "ECS/Components/ControllerSettings.h"

class InputSystem
{
public:
    InputSystem() = default;
    ~InputSystem() = default;

    InputSystem(const InputSystem&) = delete;
    InputSystem& operator=(const InputSystem&) = delete;
    InputSystem(InputSystem&&) = delete;
    InputSystem& operator=(InputSystem&&) = delete;

    // 更新
    // ControllerComponent + ControllerSettings を見て InputComponent に変換する
    void Update(entt::registry& registry, const ControllerSettings& settings);

private:
    // デッドゾーン適用（スティック用）
    float ApplyStickDeadzone(float value, float deadzone);

    // デッドゾーン適用（トリガー用）
    float ApplyTriggerDeadzone(float value, float deadzone);
};