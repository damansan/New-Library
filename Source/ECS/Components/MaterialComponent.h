#pragma once
#include <cstdint>

// 使用するGBufferシェーダーの種類
enum class ShadingModel : uint8_t
{
    Default = 0,
    Basic,      
};

struct MaterialComponent
{
    ShadingModel shadingModel = ShadingModel::Default;

    // シェーダー名に変換するヘルパー
    const char* GetShaderName() const
    {
        switch (shadingModel)
        {
        case ShadingModel::Basic:   return "gbuffer_basic";
        default:                    return "gbuffer_default";
        }
    }
};