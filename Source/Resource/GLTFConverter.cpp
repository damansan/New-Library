#include "GLTFConverter.h"

// 位置（Z反転）
DirectX::XMFLOAT3 GLTFConverter::Position(double x, double y, double z)
{
    return {
        static_cast<float>(x),
        static_cast<float>(y),
        static_cast<float>(z)  // 右手系→左手系 Z反転
    };
}
// 法線（Z反転）
DirectX::XMFLOAT3 GLTFConverter::Normal(double x, double y, double z)
{
    return {
        static_cast<float>(x),
        static_cast<float>(y),
        static_cast<float>(z)  // 右手系→左手系 Z反転
    };
}
// 平行移動（Z反転）
DirectX::XMFLOAT3 GLTFConverter::Translation(double x, double y, double z)
{
    return {
        static_cast<float>(x),
        static_cast<float>(y),
        static_cast<float>(z)  // 右手系→左手系 Z反転
    };
}


// スケール（変換不要）
DirectX::XMFLOAT3 GLTFConverter::Scale(double x, double y, double z)
{
    return {
        static_cast<float>(x),
        static_cast<float>(y),
        static_cast<float>(z)
    };
}

// クォータニオン（X,Y反転）
DirectX::XMFLOAT4 GLTFConverter::Rotation(double x, double y, double z, double w)
{
    return {
        static_cast<float>(x),  // 右手系→左手系 X反転
        static_cast<float>(y),  // 右手系→左手系 Y反転
        static_cast<float>(z),
        static_cast<float>(w)
    };
}

// 行列（転置 + Z行/列反転）
DirectX::XMFLOAT4X4 GLTFConverter::Matrix(const std::vector<double>& m)
{
    return {
            (float)m[0], (float)m[4], (float)m[8],  (float)m[12],
            (float)m[1], (float)m[5], (float)m[9],  (float)m[13],
            (float)m[2], (float)m[6], (float)m[10], (float)m[14],
            (float)m[3], (float)m[7], (float)m[11], (float)m[15]
    };
}


// アニメーション用位置（Z反転）
DirectX::XMFLOAT3 GLTFConverter::AnimPosition(float x, float y, float z)
{
    return { x, y, z };  // 右手系→左手系 Z反転
}

// アニメーション用クォータニオン（X,Y反転）
DirectX::XMFLOAT4 GLTFConverter::AnimRotation(float x, float y, float z, float w)
{
    return { x, y, z, w };  // 右手系→左手系 X,Y反転
}

// アニメーション用スケール（変換不要）
DirectX::XMFLOAT3 GLTFConverter::AnimScale(float x, float y, float z)
{
    return { x, y, z };
}