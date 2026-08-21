#pragma once

struct GameInputComponent
{
    // スティック
    float moveX = 0.0f;   // 移動
    float moveZ = 0.0f;
    float cameraX = 0.0f; // カメラ操作
    float cameraY = 0.0f;

    // トリガー
	float dodge = 0.0f; // 回避

    // アクション
    bool jump = false;
    bool attack = false;
};