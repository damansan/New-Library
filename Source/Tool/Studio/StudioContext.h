#pragma once
#include <d3d11.h>
#include <string>

// アタッチメント間で共有するデータ
struct StudioContext
{
    // Graphics系（frameworkから借りる）
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* deviceContext = nullptr;

    // アニメーション状態（アタッチメント間で共有）
    std::string currentAnimName;
    float       animTime = 0.0f;
    float       animLength = 0.0f;
    bool        isPlaying = false;

    // パネル表示フラグ
    bool showAnimator      = false;
    bool showTimeline      = false;
    bool showAnimatorGraph = false;
    bool showSequencer = false;

    std::string attachedGraphPath;
    std::string currentModelPath;

    // タイムラインでスクラブ操作されたフラグ
    bool requestPoseUpdate = false;
};