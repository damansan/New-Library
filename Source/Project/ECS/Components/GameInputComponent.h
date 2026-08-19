#pragma once

struct GameInputComponent
{
    // 移動
    float moveX = 0.0f;
    float moveZ = 0.0f;
    // カメラ操作
    float cameraX = 0.0f;
    float cameraY = 0.0f;

    // アクション
    bool jump = false;
    bool attack = false;
    bool dodge = false;
};