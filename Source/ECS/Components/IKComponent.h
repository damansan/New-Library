#pragma once
#include <DirectXMath.h>

// FootIK用のコンポーネント(現時点では動作確認用の最小構成)
struct IKComponent
{
    bool enabled = true; // IKを有効にするか

    // 足を届かせたいワールド座標(仮の地面。将来的にはレイキャストで自動取得予定)
    DirectX::XMFLOAT3 targetPos{ 0.0f, 0.0f, 0.0f };

    bool hasInitializedTarget = false;

};