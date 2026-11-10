#include "GameInputSystem.h"
#include <cmath>
#include <algorithm>

// 更新
void GameInputSystem::Update(entt::registry& registry, const ControllerSettings& settings)
{
    // ControllerComponent を探す（なければ何もしない）
    ControllerComponent* ctrl = nullptr;
    auto ctrlView = registry.view<ControllerComponent>();
    if (!ctrlView.empty())
    {
        ctrl = &ctrlView.get<ControllerComponent>(*ctrlView.begin());
    }

    // GameInputComponent を持つ Entity を更新
    auto inputView = registry.view<GameInputComponent>();
    inputView.each([&](GameInputComponent& input)
        {
            // リセット
            input.moveStick.x = 0.0f;
            input.moveStick.y = 0.0f;
            input.moveStick.hasInput = false;

            input.cameraStick.x = 0.0f;
            input.cameraStick.y = 0.0f;
            input.cameraStick.hasInput = false;

            input.lockOnTrigger.value = 0.0f;
            input.grabTrigger.value = 0.0f;

            // ControllerComponent がなければスキップ
            if (!ctrl || !ctrl->connected)
            {
                // 接続が切れた場合も、ボタンは押されていない状態にする
                UpdateButtonState(input.lockOnTrigger, false);
                UpdateButtonState(input.grabTrigger, false);
                UpdateButtonState(input.jump, false);
                UpdateButtonState(input.dodge, false);
                UpdateButtonState(input.lightAttack, false);
                UpdateButtonState(input.heavyAttack, false);
                UpdateButtonState(input.wire, false);
                return;
            }

            // スティック（デッドゾーン適用）
            float lx = ApplyStickDeadzone(ctrl->leftStickX, settings.leftStickDeadzone);
            float ly = ApplyStickDeadzone(ctrl->leftStickY, settings.leftStickDeadzone);
            float rx = ApplyStickDeadzone(ctrl->rightStickX, settings.rightStickDeadzone);
            float ry = ApplyStickDeadzone(ctrl->rightStickY, settings.rightStickDeadzone);

            // 移動（左スティック）
            input.moveStick.x = std::clamp(lx, -1.0f, 1.0f);
            input.moveStick.y = std::clamp(ly, -1.0f, 1.0f);
            input.moveStick.hasInput = (input.moveStick.x != 0.0f || input.moveStick.y != 0.0f);

            // カメラ（右スティック）
            input.cameraStick.x = rx;
            input.cameraStick.y = -ry;
            input.cameraStick.hasInput = (input.cameraStick.x != 0.0f || input.cameraStick.y != 0.0f);

            // トリガー（デッドゾーン適用後の値で判定）
            input.lockOnTrigger.value = ApplyTriggerDeadzone(ctrl->leftTrigger, settings.leftTriggerDeadzone);
            input.grabTrigger.value = ApplyTriggerDeadzone(ctrl->rightTrigger, settings.rightTriggerDeadzone);

            // トリガーの押下判定（ヒステリシス込み）
            UpdateButtonState(input.lockOnTrigger, ctrl->IsPressed(ControllerButtons::L2));
            UpdateButtonState(input.grabTrigger, ctrl->IsPressed(ControllerButtons::R2));

            // ボタン判定
            UpdateButtonState(input.jump,        ctrl->IsPressed(ControllerButtons::Cross));
            UpdateButtonState(input.dodge,       ctrl->IsPressed(ControllerButtons::Circle));
            UpdateButtonState(input.lightAttack, ctrl->IsPressed(ControllerButtons::Square));
            UpdateButtonState(input.heavyAttack, ctrl->IsPressed(ControllerButtons::Triangle));
            UpdateButtonState(input.wire,        ctrl->IsPressed(ControllerButtons::R1));
        });
}

// デッドゾーン適用
float GameInputSystem::ApplyStickDeadzone(float value, float deadzone)
{
    if (fabsf(value) < deadzone) return 0.0f;
    float sign = value > 0.0f ? 1.0f : -1.0f;
    return sign * (fabsf(value) - deadzone) / (1.0f - deadzone);
}

// デッドゾーン適用
float GameInputSystem::ApplyTriggerDeadzone(float value, float deadzone)
{
    if (value < deadzone) return 0.0f;
    return (value - deadzone) / (1.0f - deadzone);
}

// ボタン状態の更新
void GameInputSystem::UpdateButtonState(TriggerState& state, bool isHeldNow)
{
    state.pressed = isHeldNow && !state.wasHeld;
    state.released = !isHeldNow && state.wasHeld;
    state.held = isHeldNow;
    state.wasHeld = isHeldNow;
}
void GameInputSystem::UpdateButtonState(ButtonState& state, bool isHeldNow)
{
    state.pressed = isHeldNow && !state.wasHeld;
    state.released = !isHeldNow && state.wasHeld;
    state.held = isHeldNow;
    state.wasHeld = isHeldNow;
}
