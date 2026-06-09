#pragma once
struct InputComponent
{
    // 移動方向（正規化前の入力値）
    float moveX{ 0.0f };  // A/D キー  (-1 ～ +1)
    float moveZ{ 0.0f };  // W/S キー  (-1 ～ +1)

    // アクション入力(ふえてく)
    bool jump{ false };   // SPASE
    bool attack{ false }; // LMB?
};