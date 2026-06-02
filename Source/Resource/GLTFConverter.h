#pragma once
#include <DirectXMath.h>
#include <vector>


class GLTFConverter
{
public:
    // 位置・法線・平行移動（Z反転）
    static DirectX::XMFLOAT3 Position(double x, double y, double z);
    static DirectX::XMFLOAT3 Normal(double x, double y, double z);
    static DirectX::XMFLOAT3 Translation(double x, double y, double z);

    // スケール（変換不要）
    static DirectX::XMFLOAT3 Scale(double x, double y, double z);

    // クォータニオン（X,Y反転）
    static DirectX::XMFLOAT4 Rotation(double x, double y, double z, double w);

    // 行列（転置 + Z反転）
    static DirectX::XMFLOAT4X4 Matrix(const std::vector<double>& m);

    // アニメーションキーフレーム用
    static DirectX::XMFLOAT3 AnimPosition(float x, float y, float z);
    static DirectX::XMFLOAT4 AnimRotation(float x, float y, float z, float w);
    static DirectX::XMFLOAT3 AnimScale(float x, float y, float z);
};