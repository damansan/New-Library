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
            input.moveX = 0.0f;
            input.moveZ = 0.0f;
            input.cameraX = 0.0f;
            input.cameraY = 0.0f;

            input.dodge = 0.0f;

            input.jump = false;
            input.attack = false;

            // ControllerComponent がなければスキップ
            if (!ctrl || !ctrl->connected) return;

            // スティック（デッドゾーン適用）
            float lx = ApplyStickDeadzone(ctrl->leftStickX, settings.leftStickDeadzone);
            float ly = ApplyStickDeadzone(ctrl->leftStickY, settings.leftStickDeadzone);
            float rx = ApplyStickDeadzone(ctrl->rightStickX, settings.rightStickDeadzone);
            float ry = ApplyStickDeadzone(ctrl->rightStickY, settings.rightStickDeadzone);

            // 移動（左スティック）
            input.moveX = lx;
            input.moveZ = ly;

            // カメラ（右スティック）
            input.cameraX = rx;
            input.cameraY = -ry;

            // トリガー（デッドゾーン適用後の値で判定）
            float lt = ApplyTriggerDeadzone(ctrl->leftTrigger, settings.leftTriggerDeadzone);
            float rt = ApplyTriggerDeadzone(ctrl->rightTrigger, settings.rightTriggerDeadzone);

            // ボタン判定
            input.jump = ctrl->IsPressed(ControllerButtons::Cross);
            input.attack = ctrl->IsPressed(ControllerButtons::Square);

            // R2の生値をそのまま渡す
            input.dodge = rt;

            (void)lt;

            // 移動入力を -1.0～1.0 にクランプ
            input.moveX = std::clamp(input.moveX, -1.0f, 1.0f);
            input.moveZ = std::clamp(input.moveZ, -1.0f, 1.0f);
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
