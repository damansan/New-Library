#include "StudioCamera.h"
#include <imgui.h>
#include <Windows.h>
#include <algorithm>
#include <cmath>

// コンストラクタ
StudioCamera::StudioCamera()
{
    // 初期のeye/targetからyaw/pitchを計算
    DirectX::XMFLOAT3 dir =
    {
        target.x - eye.x,
        target.y - eye.y,
        target.z - eye.z
    };
    float len = sqrtf(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
    if (len > 0.0f) { dir.x /= len; dir.y /= len; dir.z /= len; }

    yaw = atan2f(dir.x, dir.z);
    pitch = asinf(dir.y);

    // 初期射影行列
    UpdateProjectionMatrix();
    UpdateViewMatrix();
}

// 更新
void StudioCamera::Update(bool isWindowHovered, float elapsedTime)
{
    // 右クリックでウィンドウ内にいるとき操作開始
    if (isWindowHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
    {
        isControlling = true;
    }
    // 右クリックを離したら操作終了
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Right))
    {
        isControlling = false;
        firstMouse = true;
        return;
    }
    if (!isControlling) return;

    float move = speed * elapsedTime;

    // ホイールで速度変更
    float wheel = ImGui::GetIO().MouseWheel;
    if (wheel != 0.0f)
    {
        speed += wheel * 2.0f;
        speed = std::clamp(speed, 0.1f, 100.0f);
    }

    DirectX::XMFLOAT3 front = GetFront();
    DirectX::XMFLOAT3 right = GetRight();

    // WASD + QE移動
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
    if (GetAsyncKeyState('D') & 0x8000)
    {
        eye.x += right.x * move; eye.y += right.y * move; eye.z += right.z * move;
        target.x += right.x * move; target.y += right.y * move; target.z += right.z * move;
    }
    if (GetAsyncKeyState('A') & 0x8000)
    {
        eye.x -= right.x * move; eye.y -= right.y * move; eye.z -= right.z * move;
        target.x -= right.x * move; target.y -= right.y * move; target.z -= right.z * move;
    }
    if (GetAsyncKeyState('Q') & 0x8000) { eye.y += move; target.y += move; }
    if (GetAsyncKeyState('E') & 0x8000) { eye.y -= move; target.y -= move; }

    // マウス回転
    POINT mousePos;
    GetCursorPos(&mousePos);

    if (firstMouse)
    {
        lastMouseX = mousePos.x;
        lastMouseY = mousePos.y;
        firstMouse = false;

        DirectX::XMFLOAT3 dir =
        {
            target.x - eye.x,
            target.y - eye.y,
            target.z - eye.z
        };
        float len = sqrtf(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
        if (len > 0.0f) { dir.x /= len; dir.y /= len; dir.z /= len; }
        pitch = asinf(dir.y);
        yaw = atan2f(dir.x, dir.z);
    }

    float dx = static_cast<float>(mousePos.x - lastMouseX) * 0.005f;
    float dy = static_cast<float>(mousePos.y - lastMouseY) * 0.005f;
    yaw += dx;
    pitch -= dy;
    pitch = std::clamp(pitch,
        -DirectX::XM_PIDIV2 + 0.1f,
        DirectX::XM_PIDIV2 - 0.1f);

    lastMouseX = mousePos.x;
    lastMouseY = mousePos.y;

    // targetを更新
    DirectX::XMFLOAT3 direction =
    {
        cosf(pitch) * sinf(yaw),
        sinf(pitch),
        cosf(pitch) * cosf(yaw)
    };
    target.x = eye.x + direction.x;
    target.y = eye.y + direction.y;
    target.z = eye.z + direction.z;

    UpdateViewMatrix();
}

// 毎フレームウィンドウサイズを渡して比率を更新する
void StudioCamera::UpdateAspect(float width, float height)
{
    if (height <= 0.0f) 
    {
        return;
    }
    aspect = width / height;
    UpdateProjectionMatrix();
}

void StudioCamera::UpdateViewMatrix()
{
    DirectX::XMVECTOR Eye = DirectX::XMLoadFloat3(&eye);
    DirectX::XMVECTOR Target = DirectX::XMLoadFloat3(&target);
    DirectX::XMVECTOR Up = DirectX::XMVectorSet(0, 1, 0, 0);
    DirectX::XMStoreFloat4x4(&view,
        DirectX::XMMatrixLookAtLH(Eye, Target, Up));
}
void StudioCamera::UpdateProjectionMatrix()
{
    DirectX::XMStoreFloat4x4(&projection,
        DirectX::XMMatrixPerspectiveFovLH(
            DirectX::XMConvertToRadians(45.0f),
            aspect, 0.1f, 1000.0f));
}

DirectX::XMFLOAT3 StudioCamera::GetFront() const
{
    DirectX::XMFLOAT3 front =
    {
        target.x - eye.x,
        target.y - eye.y,
        target.z - eye.z
    };
    float len = sqrtf(
        front.x * front.x +
        front.y * front.y +
        front.z * front.z);
    if (len > 0.0f) { front.x /= len; front.y /= len; front.z /= len; }
    return front;
}
DirectX::XMFLOAT3 StudioCamera::GetRight() const
{
    DirectX::XMFLOAT3 front = GetFront();
    DirectX::XMVECTOR Front = DirectX::XMLoadFloat3(&front);
    DirectX::XMVECTOR Up = DirectX::XMVectorSet(0, 1, 0, 0);
    DirectX::XMVECTOR Right = DirectX::XMVector3Normalize(
        DirectX::XMVector3Cross(Up, Front));
    DirectX::XMFLOAT3 right;
    DirectX::XMStoreFloat3(&right, Right);
    return right;
}