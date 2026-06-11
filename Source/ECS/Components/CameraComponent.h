#pragma once
#include "entt.hpp"
#include <DirectXMath.h>

struct CameraComponent
{
    // カメラの種類
    enum class Mode
    {
        Follow, // 追従カメラ
        LookAt, // 注視カメラ（ロックオン）
        Fixed,  // 固定カメラ
    };
    Mode mode = Mode::Follow;

    // 追従対象の Entity
    entt::entity target = entt::null;

    // 追従パラメータ
    float distance = 5.0f; // 対象からの距離
    float height = 2.0f;   // 対象からの高さ
    float yaw = 0.0f;      // 水平角
    float pitch = 0.3f;    // 垂直角（少し見下ろす）

    // 追従の滑らかさ（0に近いほど遅れて追従）
    float followSpeed = 10.0f;

    // 現在のカメラ位置（補間用）
    DirectX::XMFLOAT3 currentEye = { 0, 0, 0 };
    DirectX::XMFLOAT3 currentTarget = { 0, 0, 0 };
};