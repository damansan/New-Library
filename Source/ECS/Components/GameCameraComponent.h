#pragma once
#include <DirectXMath.h>
#include "entt.hpp"
#include "BaseCameraComponent.h"

// ゲームカメラコンポーネント
struct GameCameraComponent
{
    BaseCameraComponent base;  // 共通データ（eye, target）

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
    float distance = 5.0f;
    float height = 2.0f;
    float yaw = 0.0f;
    float pitch = 0.3f;
    float followSpeed = 10.0f;
};