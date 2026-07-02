#pragma once
#include <DirectXMath.h>

// 2BoneIKŒvŽZ
namespace TwoBoneIKSolver
{
	struct Result
	{
		DirectX::XMFLOAT4 rootGlobalRotation;
		DirectX::XMFLOAT3 midBendAxis;
		float midBendAngleDelta = 0.0f;
	};

	Result Solve(
		const DirectX::XMFLOAT3& rootPos,
		const DirectX::XMFLOAT3& midPos,
		const DirectX::XMFLOAT3& endPos,
		const DirectX::XMFLOAT3& targetPos,
		const DirectX::XMFLOAT3& poleVector,
		const DirectX::XMFLOAT4& rootGlobalRotation);
}