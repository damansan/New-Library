#include "ShapeRenderer.h"
#include "Resource/GpuResourceUtils.h"

static const float PI = DirectX::XM_PI;
static const int   SUBDIVISIONS = 16;

ShapeRenderer::ShapeRenderer(ID3D11Device* device)
{
	// Wiredシェーダー
	{
		D3D11_INPUT_ELEMENT_DESC layout[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		GpuResourceUtils::LoadVertexShader(device, "Data/Shader/WiredShape_VS.cso", layout, _countof(layout), wiredInputLayout, wiredVertexShader);
		GpuResourceUtils::LoadPixelShader(device, "Data/Shader/WiredShape_PS.cso", wiredPixelShader);
		GpuResourceUtils::CreateConstantBuffer(device, sizeof(CbWiredMesh), wiredConstantBuffer);
	}

	// Solidシェーダー
	{
		D3D11_INPUT_ELEMENT_DESC layout[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,                            D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		GpuResourceUtils::LoadVertexShader(device, "Data/Shader/SolidShape_VS.cso", layout, _countof(layout), solidInputLayout, solidVertexShader);
		GpuResourceUtils::LoadPixelShader(device, "Data/Shader/SolidShape_PS.cso", solidPixelShader);
		GpuResourceUtils::CreateConstantBuffer(device, sizeof(CbSolidMesh), solidConstantBuffer);
	}

	// 単位メッシュを事前生成
	CreateWiredBoxMesh(device);
	CreateSolidBoxMesh(device);
	CreateWiredSphereMesh(device, SUBDIVISIONS);
	CreateSolidSphereMesh(device, SUBDIVISIONS);
	CreateWiredHalfSphereMesh(device, SUBDIVISIONS);
	CreateSolidHalfSphereMesh(device, SUBDIVISIONS);
	CreateWiredCylinderMesh(device, SUBDIVISIONS);
	CreateSolidCylinderMesh(device, SUBDIVISIONS);
}

// 箱描画
void ShapeRenderer::DrawBox(
	const DirectX::XMFLOAT4X4& transform,
	const DirectX::XMFLOAT3& size,
	const DirectX::XMFLOAT4& color)
{
	DirectX::XMMATRIX S = DirectX::XMMatrixScaling(size.x, size.y, size.z);
	DirectX::XMMATRIX W = DirectX::XMLoadFloat4x4(&transform);

	Instance inst;
	inst.wiredMesh = &wiredBoxMesh;
	inst.solidMesh = &solidBoxMesh;
	DirectX::XMStoreFloat4x4(&inst.worldTransform, S * W);
	inst.color = color;
	instances.push_back(inst);
}
// 球描画
void ShapeRenderer::DrawSphere(
	const DirectX::XMFLOAT3& center,
	float radius,
	const DirectX::XMFLOAT4& color)
{
	DirectX::XMMATRIX S = DirectX::XMMatrixScaling(radius, radius, radius);
	DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(center.x, center.y, center.z);

	Instance inst;
	inst.wiredMesh = &wiredSphereMesh;
	inst.solidMesh = &solidSphereMesh;
	DirectX::XMStoreFloat4x4(&inst.worldTransform, S * T);
	inst.color = color;
	instances.push_back(inst);
}
// カプセル描画
void ShapeRenderer::DrawCapsule(
	const DirectX::XMFLOAT4X4& transform,
	float radius, 
	float halfHeight,
	const DirectX::XMFLOAT4& color)
{
	DirectX::XMMATRIX base = DirectX::XMLoadFloat4x4(&transform);
	DirectX::XMMATRIX S = DirectX::XMMatrixScaling(radius, radius, radius);
	DirectX::XMMATRIX flip = DirectX::XMMatrixScaling(1.0f, -1.0f, 1.0f);

	// 上半球
	{
		DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(0, halfHeight, 0);
		Instance inst;
		inst.wiredMesh = &wiredHalfSphereMesh;
		inst.solidMesh = &solidHalfSphereMesh;
		DirectX::XMStoreFloat4x4(&inst.worldTransform, S * T * base);
		inst.color = color;
		instances.push_back(inst);
	}
	// 下半球
	{
		DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(0, -halfHeight, 0);
		Instance inst;
		inst.wiredMesh = &wiredHalfSphereMesh;
		inst.solidMesh = &solidHalfSphereMesh;
		DirectX::XMStoreFloat4x4(&inst.worldTransform, S * flip * T * base);
		inst.color = color;
		instances.push_back(inst);
	}
	// 円柱
	{
		DirectX::XMMATRIX cylS = DirectX::XMMatrixScaling(radius, halfHeight * 2.0f, radius);
		Instance inst;
		inst.wiredMesh = &wiredCylinderMesh;
		inst.solidMesh = &solidCylinderMesh;
		DirectX::XMStoreFloat4x4(&inst.worldTransform, cylS * base);
		inst.color = color;
		instances.push_back(inst);
	}
}

// 描画
void ShapeRenderer::Render(
	ID3D11DeviceContext* dc,
	const DirectX::XMFLOAT4X4& view,
	const DirectX::XMFLOAT4X4& projection,
	const DirectX::XMFLOAT3& lightDirection)
{
	if (instances.empty()) return;

	DirectX::XMMATRIX V = DirectX::XMLoadFloat4x4(&view);
	DirectX::XMMATRIX P = DirectX::XMLoadFloat4x4(&projection);
	DirectX::XMMATRIX VP = V * P;

	// Wiredパス
	dc->VSSetShader(wiredVertexShader.Get(), nullptr, 0);
	dc->PSSetShader(wiredPixelShader.Get(), nullptr, 0);
	dc->IASetInputLayout(wiredInputLayout.Get());
	dc->VSSetConstantBuffers(0, 1, wiredConstantBuffer.GetAddressOf());
	dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	for (auto& inst : instances)
	{
		DirectX::XMMATRIX W = DirectX::XMLoadFloat4x4(&inst.worldTransform);

		CbWiredMesh cb;
		DirectX::XMStoreFloat4x4(&cb.worldViewProjection, W * VP);
		cb.color = inst.color;
		dc->UpdateSubresource(wiredConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);

		UINT stride = sizeof(DirectX::XMFLOAT3);
		UINT offset = 0;
		dc->IASetVertexBuffers(0, 1, inst.wiredMesh->vertexBuffer.GetAddressOf(), &stride, &offset);
		dc->Draw(inst.wiredMesh->vertexCount, 0);
	}

	// Solidパス
	dc->VSSetShader(solidVertexShader.Get(), nullptr, 0);
	dc->PSSetShader(solidPixelShader.Get(), nullptr, 0);
	dc->IASetInputLayout(solidInputLayout.Get());
	dc->VSSetConstantBuffers(0, 1, solidConstantBuffer.GetAddressOf());
	dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	for (auto& inst : instances)
	{
		DirectX::XMMATRIX W = DirectX::XMLoadFloat4x4(&inst.worldTransform);

		CbSolidMesh cb;
		DirectX::XMStoreFloat4x4(&cb.world, W);
		DirectX::XMStoreFloat4x4(&cb.viewProjection, VP);
		cb.lightDirection = { lightDirection.x, lightDirection.y, lightDirection.z, 0 };
		cb.color = inst.color;
		dc->UpdateSubresource(solidConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);

		UINT stride = sizeof(Vertex);
		UINT offset = 0;
		dc->IASetVertexBuffers(0, 1, inst.solidMesh->vertexBuffer.GetAddressOf(), &stride, &offset);
		dc->IASetIndexBuffer(inst.solidMesh->indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
		dc->DrawIndexed(inst.solidMesh->indexCount, 0, 0);
	}

	instances.clear();
}

// バッファ生成ヘルパー
void ShapeRenderer::CreateWiredMesh(
	ID3D11Device* device,
	const std::vector<DirectX::XMFLOAT3>& positions,
	WiredMesh& mesh)
{
	D3D11_BUFFER_DESC desc = {};
	desc.ByteWidth = static_cast<UINT>(sizeof(DirectX::XMFLOAT3) * positions.size());
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA init = {};
	init.pSysMem = positions.data();

	HRESULT hr = device->CreateBuffer(&desc, &init, mesh.vertexBuffer.GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), L"ShapeRenderer: WiredMesh failed");
	mesh.vertexCount = static_cast<UINT>(positions.size());
}
void ShapeRenderer::CreateSolidMesh(
	ID3D11Device* device, 
	const std::vector<Vertex>& vertices,
	const std::vector<uint16_t>& indices,
	SolidMesh& mesh)
{
	{
		D3D11_BUFFER_DESC desc = {};
		desc.ByteWidth = static_cast<UINT>(sizeof(Vertex) * vertices.size());
		desc.Usage = D3D11_USAGE_IMMUTABLE;
		desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

		D3D11_SUBRESOURCE_DATA init = {};
		init.pSysMem = vertices.data();

		HRESULT hr = device->CreateBuffer(&desc, &init, mesh.vertexBuffer.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), L"ShapeRenderer: SolidMesh vertex failed");
	}

	{
		D3D11_BUFFER_DESC desc = {};
		desc.ByteWidth = static_cast<UINT>(sizeof(uint16_t) * indices.size());
		desc.Usage = D3D11_USAGE_IMMUTABLE;
		desc.BindFlags = D3D11_BIND_INDEX_BUFFER;

		D3D11_SUBRESOURCE_DATA init = {};
		init.pSysMem = indices.data();

		HRESULT hr = device->CreateBuffer(&desc, &init, mesh.indexBuffer.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), L"ShapeRenderer: SolidMesh index failed");
	}
	mesh.indexCount = static_cast<UINT>(indices.size());
}

// 箱
void ShapeRenderer::CreateWiredBoxMesh(ID3D11Device* device)
{
	float h = 0.5f;
	std::vector<DirectX::XMFLOAT3> p = 
	{
		// Bottom
		{-h,-h,-h},{h,-h,-h},  {h,-h,-h},{h,-h,h},
		{h,-h,h},{-h,-h,h},    {-h,-h,h},{-h,-h,-h},
		// Top
		{-h,h,-h},{h,h,-h},    {h,h,-h},{h,h,h},
		{h,h,h},{-h,h,h},      {-h,h,h},{-h,h,-h},
		// Verticals
		{-h,-h,-h},{-h,h,-h},  {h,-h,-h},{h,h,-h},
		{h,-h,h},{h,h,h},      {-h,-h,h},{-h,h,h},
	};
	CreateWiredMesh(device, p, wiredBoxMesh);
}
void ShapeRenderer::CreateSolidBoxMesh(ID3D11Device* device)
{
	float h = 0.5f;
	std::vector<Vertex> verts = 
	{
		{{-h,-h,h},  {0,0,1}}, {{h,-h,h},  {0,0,1}},  {{h,h,h},  {0,0,1}},  {{-h,h,h}, {0,0,1}},  // +Z
		{{h,-h,-h},  {0,0,-1}},{{-h,-h,-h},{0,0,-1}}, {{-h,h,-h},{0,0,-1}}, {{h,h,-h}, {0,0,-1}}, // -Z
		{{-h,-h,-h}, {-1,0,0}},{{-h,-h,h}, {-1,0,0}}, {{-h,h,h}, {-1,0,0}}, {{-h,h,-h},{-1,0,0}}, // -X
		{{h,-h,h},   {1,0,0}}, {{h,-h,-h}, {1,0,0}},  {{h,h,-h}, {1,0,0}},  {{h,h,h},  {1,0,0}},  // +X
		{{-h,h,h},   {0,1,0}}, {{h,h,h},   {0,1,0}},  {{h,h,-h}, {0,1,0}},  {{-h,h,-h},{0,1,0}},  // +Y
		{{-h,-h,-h}, {0,-1,0}},{{h,-h,-h}, {0,-1,0}}, {{h,-h,h}, {0,-1,0}}, {{-h,-h,h},{0,-1,0}}, // -Y
	};
	std::vector<uint16_t> idxs;
	for (uint16_t f = 0; f < 6; f++)
	{
		uint16_t b = f * 4;
		idxs.push_back(b); idxs.push_back(b + 1); idxs.push_back(b + 2);
		idxs.push_back(b); idxs.push_back(b + 2); idxs.push_back(b + 3);
	}
	CreateSolidMesh(device, verts, idxs, solidBoxMesh);
}

// 球
void ShapeRenderer::CreateWiredSphereMesh(ID3D11Device* device, int subdivisions)
{
	std::vector<DirectX::XMFLOAT3> p;
	for (int i = 0; i < subdivisions; i++) 
	{
		float a0 = 2.0f * PI * i / subdivisions;
		float a1 = 2.0f * PI * (i + 1) / subdivisions;
		// XY平面
		p.push_back({ cosf(a0), sinf(a0), 0 });
		p.push_back({ cosf(a1), sinf(a1), 0 });
		// XZ平面
		p.push_back({ cosf(a0), 0, sinf(a0) });
		p.push_back({ cosf(a1), 0, sinf(a1) });
		// YZ平面
		p.push_back({ 0, sinf(a0), cosf(a0) });
		p.push_back({ 0, sinf(a1), cosf(a1) });
	}
	CreateWiredMesh(device, p, wiredSphereMesh);
}
void ShapeRenderer::CreateSolidSphereMesh(ID3D11Device* device, int subdivisions)
{
	std::vector<Vertex> verts;
	std::vector<uint16_t> idxs;
	int rings = subdivisions / 2;
	int slices = subdivisions;

	for (int r = 0; r <= rings; r++)
	{
		float phi = PI * r / rings - PI * 0.5f;
		float y = sinf(phi);
		float xz = cosf(phi);
		for (int s = 0; s <= slices; s++) 
		{
			float theta = 2.0f * PI * s / slices;
			float x = xz * cosf(theta);
			float z = xz * sinf(theta);
			verts.push_back({ {x,y,z},{x,y,z} });
		}
	}
	for (int r = 0; r < rings; r++) 
	{
		for (int s = 0; s < slices; s++) 
		{
			uint16_t a = (uint16_t)(r * (slices + 1) + s);
			uint16_t b = a + 1;
			uint16_t c = a + (uint16_t)(slices + 1);
			uint16_t d = c + 1;
			idxs.push_back(a); idxs.push_back(c); idxs.push_back(b);
			idxs.push_back(b); idxs.push_back(c); idxs.push_back(d);
		}
	}
	CreateSolidMesh(device, verts, idxs, solidSphereMesh);
}

// 半球
void ShapeRenderer::CreateWiredHalfSphereMesh(ID3D11Device* device, int subdivisions)
{
	std::vector<DirectX::XMFLOAT3> p;
	for (int i = 0; i < subdivisions; i++)
	{
		float a0 = PI * i / subdivisions;
		float a1 = PI * (i + 1) / subdivisions;
		p.push_back({ cosf(a0), sinf(a0), 0 });
		p.push_back({ cosf(a1), sinf(a1), 0 });
		p.push_back({ 0, sinf(a0), cosf(a0) });
		p.push_back({ 0, sinf(a1), cosf(a1) });
	}
	for (int i = 0; i < subdivisions; i++) 
	{
		float a0 = 2.0f * PI * i / subdivisions;
		float a1 = 2.0f * PI * (i + 1) / subdivisions;
		p.push_back({ cosf(a0), 0, sinf(a0) });
		p.push_back({ cosf(a1), 0, sinf(a1) });
	}
	CreateWiredMesh(device, p, wiredHalfSphereMesh);
}
void ShapeRenderer::CreateSolidHalfSphereMesh(ID3D11Device* device, int subdivisions)
{
	std::vector<Vertex> verts;
	std::vector<uint16_t> idxs;
	int rings = subdivisions / 4;
	int slices = subdivisions;

	for (int r = 0; r <= rings; r++) 
	{
		float phi = (PI * 0.5f) * r / rings;
		float y = sinf(phi);
		float xz = cosf(phi);
		for (int s = 0; s <= slices; s++) 
		{
			float theta = 2.0f * PI * s / slices;
			float x = xz * cosf(theta);
			float z = xz * sinf(theta);
			verts.push_back({ {x,y,z},{x,y,z} });
		}
	}
	for (int r = 0; r < rings; r++) 
	{
		for (int s = 0; s < slices; s++) 
		{
			uint16_t a = (uint16_t)(r * (slices + 1) + s);
			uint16_t b = a + 1;
			uint16_t c = a + (uint16_t)(slices + 1);
			uint16_t d = c + 1;
			idxs.push_back(a); idxs.push_back(c); idxs.push_back(b);
			idxs.push_back(b); idxs.push_back(c); idxs.push_back(d);
		}
	}
	CreateSolidMesh(device, verts, idxs, solidHalfSphereMesh);
}

// 円柱
void ShapeRenderer::CreateWiredCylinderMesh(ID3D11Device* device, int subdivisions)
{
	std::vector<DirectX::XMFLOAT3> p;
	for (int i = 0; i < subdivisions; i++) 
	{
		float a0 = 2.0f * PI * i / subdivisions;
		float a1 = 2.0f * PI * (i + 1) / subdivisions;
		// 上リング
		p.push_back({ cosf(a0),  0.5f, sinf(a0) });
		p.push_back({ cosf(a1),  0.5f, sinf(a1) });
		// 下リング
		p.push_back({ cosf(a0), -0.5f, sinf(a0) });
		p.push_back({ cosf(a1), -0.5f, sinf(a1) });
	}
	// 4本縦ライン
	for (int i = 0; i < 4; i++) 
	{
		float a = 2.0f * PI * i / 4;
		float x = cosf(a), z = sinf(a);
		p.push_back({ x,  0.5f, z });
		p.push_back({ x, -0.5f, z });
	}
	CreateWiredMesh(device, p, wiredCylinderMesh);
}
void ShapeRenderer::CreateSolidCylinderMesh(ID3D11Device* device, int subdivisions)
{
	std::vector<Vertex> verts;
	std::vector<uint16_t> idxs;

	// 側面のみ
	for (int i = 0; i <= subdivisions; i++) 
	{
		float theta = 2.0f * PI * i / subdivisions;
		float x = cosf(theta), z = sinf(theta);
		verts.push_back({ {x,  0.5f, z}, {x, 0, z} });
		verts.push_back({ {x, -0.5f, z}, {x, 0, z} });
	}
	for (int i = 0; i < subdivisions; i++) 
	{
		uint16_t t0 = (uint16_t)(i * 2);
		uint16_t b0 = t0 + 1, t1 = t0 + 2, b1 = t0 + 3;
		idxs.push_back(t0); idxs.push_back(b0); idxs.push_back(t1);
		idxs.push_back(t1); idxs.push_back(b0); idxs.push_back(b1);
	}
	CreateSolidMesh(device, verts, idxs, solidCylinderMesh);
}
