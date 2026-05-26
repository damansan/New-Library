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

    // ボーン行列(シェーダーに渡す）
    std::vector<DirectX::XMFLOAT4X4> boneTransforms;
};