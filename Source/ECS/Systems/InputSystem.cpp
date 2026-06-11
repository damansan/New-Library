#include "InputSystem.h"
#include "ECS/Components/InputComponent.h"

void InputSystem::Update(entt::registry& registry)
{
    // XInput でコントローラー状態を取得（0番のみ）
    XINPUT_STATE xState = {};
    bool controllerConnected =
        XInputGetState(0, &xState) == ERROR_SUCCESS;

    auto view = registry.view<InputComponent>();
    view.each([&](auto entity, InputComponent& input)
        {
            // リセット
            input.moveX = 0.0f;
            input.moveZ = 0.0f;
            input.cameraX = 0.0f;
            input.cameraY = 0.0f;
            input.jump = false;
            input.attack = false;
            input.dodge = false;

            // コントローラー入力（XInput）
            if (controllerConnected)
            {
                const auto& gp = xState.Gamepad;

                // 左スティック（移動）
                float lx = ApplyDeadzone(
                    gp.sThumbLX / 32767.0f);
                float ly = ApplyDeadzone(
                    gp.sThumbLY / 32767.0f);

                input.moveX += lx;
                input.moveZ += ly;

                // 右スティック（カメラ）
                float rx = ApplyDeadzone(
                    gp.sThumbRX / 32767.0f);
                float ry = ApplyDeadzone(
                    gp.sThumbRY / 32767.0f);

                input.cameraX += rx;
                input.cameraY += -ry; // Y 軸は反転

                // ボタン
                input.jump |= (gp.wButtons & XINPUT_GAMEPAD_A) != 0;
                input.attack |= (gp.wButtons & XINPUT_GAMEPAD_X) != 0;
                input.dodge |= (gp.wButtons & XINPUT_GAMEPAD_B) != 0;
            }

            // 移動入力を -1.0〜1.0 にクランプ
            input.moveX = std::clamp(input.moveX, -1.0f, 1.0f);
            input.moveZ = std::clamp(input.moveZ, -1.0f, 1.0f);
        });
}

// デッドゾーン処理
float InputSystem::ApplyDeadzone(float value, float deadzone)
{
    if (fabsf(value) < deadzone) return 0.0f;
    // デッドゾーン外の値を 0.0～1.0 に正規化
    float sign = value > 0.0f ? 1.0f : -1.0f;
    return sign * (fabsf(value) - deadzone) / (1.0f - deadzone);
}
