#pragma once
#include <string>
#include <DirectXMath.h>

struct IKComponent
{
	// IK—LŒøƒtƒ‰ƒO
	bool enabled = false;

	std::string rootBoneName;
	std::string midBoneName;
	std::string endBoneName;

	DirectX::XMFLOAT3 targetPosition{ 0.0f, 0.0f, 0.0f };
};