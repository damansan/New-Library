#pragma once
#include <memory>
#include <vector>
#include <DirectXMath.h>
#include "Resource/Model.h"

// スキニングメッシュ用コンポーネント
struct SkinnedMeshComponent
{
    // モデルデータ
    std::shared_ptr<Model> model; 

    // メッシュごとのボーン行列
    std::vector<std::vector<DirectX::XMFLOAT4X4>> boneTransforms;

    // ノードごとのグローバル行列
    std::vector<DirectX::XMFLOAT4X4> nodeGlobalTransforms;
};