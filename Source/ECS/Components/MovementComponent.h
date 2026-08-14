#pragma once
#include <DirectXMath.h>

struct MovementComponent
{
    // 重力・ジャンプ
    float jumpPower{ 8.0f };
    float gravity{ -20.0f };

    // 現在の速度ベクトル（Systemが毎フレーム書き換える）
    DirectX::XMFLOAT3 velocity{ 0.0f, 0.0f, 0.0f };

    // 地面に接しているか
    bool isGrounded{ false };

	// 移動をロックするかどうか
    bool movementLocked{ false };
};