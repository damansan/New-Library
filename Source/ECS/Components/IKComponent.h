#pragma once
#include <string>
#include <DirectXMath.h>

struct IKComponent
{
	bool        enabled = false; // IK有効フラグ
	std::string targetBoneName;  // 操作対象のボーン名

	DirectX::XMFLOAT3 manualPosition{ 0.0f, 0.0f, 0.0f };       // 位置オフセット
	DirectX::XMFLOAT3 manualRotationEuler{ 0.0f, 0.0f, 0.0f };  // 回転(オイラー角)
};