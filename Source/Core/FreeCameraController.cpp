#include "FreeCameraController.h"
#include "imgui.h"
#include <Windows.h>
#include <algorithm>

void FreeCameraController::Update(float elapsedTime, Camera& camera)
{
    // ImGuiウィンドウ操作中は処理しない
    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow)) return;

    DirectX::XMFLOAT3 eye = camera.GetEye();
    DirectX::XMFLOAT3 target = camera.GetTarget();

    const DirectX::XMFLOAT3& front = camera.GetFront();
    const DirectX::XMFLOAT3& right = camera.GetRight();

    float move = cameraSpeed * elapsedTime;

    if (GetAsyncKeyState(VK_RBUTTON) & 0x8000)
    {
        // ホイールで速度変更
        float wheel = ImGui::GetIO().MouseWheel;
        if (wheel != 0.0f)
        {
            cameraSpeed += wheel * 2.0f;
            cameraSpeed = std::clamp(cameraSpeed, 0.1f, 100.0f);
        }

        // 前後移動（W/S）
        if (GetAsyncKeyState('W') & 0x8000)
        {
            eye.x += front.x * move; eye.y += front.y * move; eye.z += front.z * move;
            target.x += front.x * move; target.y += front.y * move; target.z += front.z * move;
        }
        if (GetAsyncKeyState('S') & 0x8000)
        {
            eye.x -= front.x * move; eye.y -= front.y * move; eye.z -= front.z * move;
            target.x -= front.x * move; target.y -= front.y * move; target.z -= front.z * move;
        }

        // 左右移動（A/D）
        if (GetAsyncKeyState('A') & 0x8000)
        {
            eye.x -= right.x * move; eye.y -= right.y * move; eye.z -= right.z * move;
            target.x -= right.x * move; target.y -= right.y * move; target.z -= right.z * move;
        }
        if (GetAsyncKeyState('D') & 0x8000)
        {
            eye.x += right.x * move; eye.y += right.y * move; eye.z += right.z * move;
            target.x += right.x * move; target.y += right.y * move; target.z += right.z * move;
        }

        // 上下移動（Q/E）
        if (GetAsyncKeyState('Q') & 0x8000) { eye.y += move; target.y += move; }
        if (GetAsyncKeyState('E') & 0x8000) { eye.y -= move; target.y -= move; }

        // マウス視点回転
        POINT mousePos;
        GetCursorPos(&mousePos);

        if (firstMouse)
        {
            lastMousePos = mousePos;
            firstMouse = false;

            DirectX::XMFLOAT3 dir = {
                target.x - eye.x,
                target.y - eye.y,
                target.z - eye.z
            };
            float len = sqrtf(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
            dir.x /= len; dir.y /= len; dir.z /= len;
            pitch = asinf(dir.y);
            yaw = atan2f(dir.x, dir.z);
        }

        float dx = static_cast<float>(mousePos.x - lastMousePos.x) * 0.005f;
        float dy = static_cast<float>(mousePos.y - lastMousePos.y) * 0.005f;

        yaw += dx;
        pitch -= dy;
        pitch = std::clamp(pitch,
            -DirectX::XM_PIDIV2 + 0.1f,
            DirectX::XM_PIDIV2 - 0.1f);

        lastMousePos = mousePos;

        // 前方向ベクトル更新
        DirectX::XMFLOAT3 direction;
        direction.x = cosf(pitch) * sinf(yaw);
        direction.y = sinf(pitch);
        direction.z = cosf(pitch) * cosf(yaw);

        target.x = eye.x + direction.x;
        target.y = eye.y + direction.y;
        target.z = eye.z + direction.z;
    }
    else
    {
        firstMouse = true;
    }

    camera.SetLookAt(eye, target, DirectX::XMFLOAT3(0, 1, 0));
}