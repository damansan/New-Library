#pragma once
#include <d3d11.h>
#include <wrl.h>
#include <DirectXMath.h>
#include "PrimitiveRenderer.h"
#include "ShapeRenderer.h"
#include <PxPhysicsAPI.h>

class PhysicsDebugRenderer
{
public:
	PhysicsDebugRenderer() = default;
	void Initialize(ID3D11Device* device);

	void Render(
		ID3D11DeviceContext* dc,
		physx::PxScene* pxScene,
		physx::PxControllerManager* pxControllerManager,
		const DirectX::XMFLOAT4X4& view,
		const DirectX::XMFLOAT4X4& projection,
		const DirectX::XMFLOAT4& lightDirection);

private:
	PrimitiveRenderer primitiveRenderer;
	ShapeRenderer     shapeRenderer;

	Microsoft::WRL::ComPtr<ID3D11BlendState>        blendState;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depthStencilState;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState>   rasterizerState;
};