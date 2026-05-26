#pragma once
#include <DirectXMath.h>

struct TransformComponent
{
	DirectX::XMFLOAT3 position{ 0.0f,0.0f,0.0f }; // 位置
	DirectX::XMFLOAT3 rotation{ 0.0f,0.0f,0.0f }; // 回転(オイラー角)
	DirectX::XMFLOAT3 scale   { 1.0f,1.0f,1.0f }; // スケール


	// ワールド行列取得
	DirectX::XMMATRIX GetWorldMatrix() const
	{
		DirectX::XMMATRIX S = DirectX::XMMatrixScaling(scale.x, scale.y, scale.z);
		DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);
		DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(position.x, position.y, position.z);
		return S * R * T;
	}
};