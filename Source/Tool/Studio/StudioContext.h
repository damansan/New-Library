#pragma once
#include <d3d11.h>
#include <string>
#include <memory>

// 前方宣言
struct GLTFData;

// アタッチメント間で共有するデータ
struct StudioContext
{
    // Graphics系（frameworkから借りる）
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* deviceContext = nullptr;

    // モデルデータ（GameのECSとは独立して持つ）
    std::shared_ptr<GLTFData> model;

    // アニメーション状態（アタッチメント間で共有）
    std::string currentAnimName;
    float       animTime = 0.0f;
    float       animLength = 0.0f;
    bool        isPlaying = false;
};