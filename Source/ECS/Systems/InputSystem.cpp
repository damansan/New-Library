#include "InputSystem.h"
#include <cmath>
#include <algorithm>

InputSystem::InputSystem()
{
    // PS5 コントローラーの初期検索
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
InputSystem::~InputSystem()
{
    if (ds5Connected)
    {
        DS5W::freeDeviceContext(&ds5Context);
        ds5Connected = false;
    }
}

void InputSystem::Update(entt::registry& registry, float elapsedTime)
{
    // Xbox の接続確認（毎フレーム）
    xboxConnected = XInputGetState(0, &xState) == ERROR_SUCCESS;

    // PS5 の入力取得（毎フレーム）
    if (ds5Connected)
    {
        DS5W_ReturnValue rv = DS5W::getDeviceInputState(&ds5Context, &ds5State);
        if (DS5W_FAILED(rv))
        {
            // 切断されたら再接続を試みる
            ds5Connected = false;
            DS5W::freeDeviceContext(&ds5Context);
        }
    }
    else
    {
        // タイマーで再検索頻度を制限
        ds5ReconnectTimer += elapsedTime;
        if (ds5ReconnectTimer >= ds5ReconnectInterval)
        {
            ds5ReconnectTimer = 0.0f;

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
    }

    // 各 Entity の InputComponent を更新
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

            UpdateXbox(input);
            UpdatePS5(input);

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

// Xbox 入力
void InputSystem::UpdateXbox(InputComponent& input)
{
    if (!xboxConnected) return;

    const auto& gp = xState.Gamepad;

    // 左スティック（移動）
    float lx = ApplyDeadzone(gp.sThumbLX / 32767.0f);
    float ly = ApplyDeadzone(gp.sThumbLY / 32767.0f);
    input.moveX += lx;
    input.moveZ += ly;

    // 右スティック（カメラ）
    float rx = ApplyDeadzone(gp.sThumbRX / 32767.0f);
    float ry = ApplyDeadzone(gp.sThumbRY / 32767.0f);
    input.cameraX += rx;
    input.cameraY += -ry;

    // ボタン
    input.jump |= (gp.wButtons & XINPUT_GAMEPAD_A) != 0;
    input.attack |= (gp.wButtons & XINPUT_GAMEPAD_X) != 0;
    input.dodge |= (gp.wButtons & XINPUT_GAMEPAD_B) != 0;
}

// PS5 入力
void InputSystem::UpdatePS5(InputComponent& input)
{
    if (!ds5Connected) return;

    // 左スティック（移動）-128〜127 を -1.0〜1.0 に変換
    float lx = ApplyDeadzone(ds5State.leftStick.x / 127.0f);
    float ly = ApplyDeadzone(ds5State.leftStick.y / 127.0f);
    input.moveX += lx;
    input.moveZ += -ly; // Y 軸反転

    // 右スティック（カメラ）
    float rx = ApplyDeadzone(ds5State.rightStick.x / 127.0f);
    float ry = ApplyDeadzone(ds5State.rightStick.y / 127.0f);
    input.cameraX += rx;
    input.cameraY += ry;

    // ボタン（× = ジャンプ、□ = 攻撃、○ = 回避）
    input.jump |= (ds5State.buttonsAndDpad & DS5W_ISTATE_BTX_CROSS) != 0;
    input.attack |= (ds5State.buttonsAndDpad & DS5W_ISTATE_BTX_SQUARE) != 0;
    input.dodge |= (ds5State.buttonsAndDpad & DS5W_ISTATE_BTX_CIRCLE) != 0;
}
