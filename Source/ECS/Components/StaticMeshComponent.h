#pragma once
#include <memory>
#include "Resource/Model.h"

// 静的メッシュ用コンポーネント
struct StaticMeshComponent
{
    // モデルデータ
    std::shared_ptr<Model> model; 
};