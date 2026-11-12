#include "RenderSystem.h"
#include "Engine/Resource/GpuResourceUtils.h"
#include "Engine/Utility/misc.h"


// 初期化
void RenderSystem::Initialize(ID3D11Device* device)
{
	// 入力レイアウト
	const std::vector<D3D11_INPUT_ELEMENT_DESC>& inputElementDescs = Model::InputElementDescs;


	// 頂点シェーダー
	GpuResourceUtils::LoadVertexShader(
		device, "Data/Shader/Static_VS.cso",
		inputElementDescs.data(), static_cast<UINT>(inputElementDescs.size()),
		inputLayout, staticVertexShader
	);
	// 頂点シェーダー(スキニング)
	Microsoft::WRL::ComPtr<ID3D11InputLayout> dummyLayout;
	GpuResourceUtils::LoadVertexShader(
		device, "Data/Shader/Skinning_VS.cso",
		inputElementDescs.data(), static_cast<UINT>(inputElementDescs.size()),
		dummyLayout, skinnedVertexShader
	);

	// GBuffer用ピクセルシェーダー
	Microsoft::WRL::ComPtr<ID3D11PixelShader> psDefault;
	GpuResourceUtils::LoadPixelShader(device, "Data/Shader/Gbuffer_PS.cso", psDefault);
	gBufferPixelShaders["gbuffer_default"] = psDefault;

	Microsoft::WRL::ComPtr<ID3D11PixelShader> psBasic;
	GpuResourceUtils::LoadPixelShader(device, "Data/Shader/Gbuffer_Basic_PS.cso", psBasic);
	gBufferPixelShaders["gbuffer_basic"] = psBasic;

	// Lightingパス用シェーダー
	Microsoft::WRL::ComPtr<ID3D11InputLayout> dummyLayoutL;
	GpuResourceUtils::LoadVertexShader(
		device, "Data/Shader/Lighting_VS.cso",
		nullptr, 0, dummyLayoutL, lightingVS
	);
	GpuResourceUtils::LoadPixelShader(
		device, "Data/Shader/Lighting_PS.cso", lightingPS
	);

	// 半透明フォワード用ピクセルシェーダー(PBR)
	GpuResourceUtils::LoadPixelShader(device, "Data/Shader/PBR_PS.cso", forwardTransparentPS);

	// 定数バッファ
	GpuResourceUtils::CreateConstantBuffer(device, sizeof(CbScene), cbScene);
	GpuResourceUtils::CreateConstantBuffer(device, sizeof(CbObject), cbObject);
	GpuResourceUtils::CreateConstantBuffer(device, sizeof(CbSkinning), cbSkinning);

	//// スキニング用は毎メッシュ書き換えるのでDYNAMICで作る
	//{
	//	D3D11_BUFFER_DESC desc{};
	//	desc.ByteWidth = sizeof(CbSkinning);
	//	desc.Usage = D3D11_USAGE_DYNAMIC;
	//	desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	//	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	//	device->CreateBuffer(&desc, nullptr, cbSkinning.GetAddressOf());
	//}

	// サンプラーステート
	D3D11_SAMPLER_DESC samplerDesc{};
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	device->CreateSamplerState(&samplerDesc, samplerState.GetAddressOf());

	// ラスタライザーステート(裏面カリング）
	D3D11_RASTERIZER_DESC rasterizerDesc{};
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerDesc.CullMode = D3D11_CULL_BACK;
	rasterizerDesc.FrontCounterClockwise = FALSE;
	device->CreateRasterizerState(&rasterizerDesc, rasterizerState.GetAddressOf());

	// ラスタライザーステート(カリングなし・フルスクリーン用）
	D3D11_RASTERIZER_DESC noCullDesc{};
	noCullDesc.FillMode = D3D11_FILL_SOLID;
	noCullDesc.CullMode = D3D11_CULL_NONE;
	noCullDesc.FrontCounterClockwise = FALSE;
	device->CreateRasterizerState(&noCullDesc, rasterizerStateNoCull.GetAddressOf());

	// 深度ステンシルステート
	D3D11_DEPTH_STENCIL_DESC dsDesc{};
	dsDesc.DepthEnable = TRUE;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dsDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	device->CreateDepthStencilState(&dsDesc, depthStencilState.GetAddressOf());

	// 深度ステンシルステート(半透明用・深度書き込みOFF）
	D3D11_DEPTH_STENCIL_DESC dsTransparent{};
	dsTransparent.DepthEnable = TRUE;
	dsTransparent.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	dsTransparent.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	device->CreateDepthStencilState(&dsTransparent, depthStencilStateTransparent.GetAddressOf());

	// 深度ステンシルステート(GeometryPass用)
	D3D11_DEPTH_STENCIL_DESC dsGeom{};
	dsGeom.DepthEnable = TRUE;
	dsGeom.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dsGeom.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	dsGeom.StencilEnable = TRUE;
	dsGeom.StencilWriteMask = 0xFF;
	dsGeom.StencilReadMask = 0xFF;
	dsGeom.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	dsGeom.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
	dsGeom.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsGeom.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	dsGeom.BackFace = dsGeom.FrontFace;
	device->CreateDepthStencilState(&dsGeom, depthStencilStateGeometry.GetAddressOf());

	// 深度ステンシルステート(LightingPass用)
	D3D11_DEPTH_STENCIL_DESC dsLight{};
	dsLight.DepthEnable = FALSE;
	dsLight.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	dsLight.StencilEnable = TRUE;
	dsLight.StencilReadMask = 0xFF;
	dsLight.StencilWriteMask = 0x00;
	dsLight.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
	dsLight.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dsLight.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsLight.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	dsLight.BackFace = dsLight.FrontFace;
	device->CreateDepthStencilState(&dsLight, depthStencilStateLighting.GetAddressOf());

	// ブレンドステート(アルファブレンド・半透明用）
	D3D11_BLEND_DESC blendDesc{};
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].RenderTargetWriteMask =
		D3D11_COLOR_WRITE_ENABLE_RED | D3D11_COLOR_WRITE_ENABLE_GREEN | D3D11_COLOR_WRITE_ENABLE_BLUE;
	device->CreateBlendState(&blendDesc, blendState.GetAddressOf());

	// ブレンドステート(ブレンドなし・GBuffer用）
	D3D11_BLEND_DESC blendDescOpaque{};
	blendDescOpaque.RenderTarget[0].BlendEnable = FALSE;
	blendDescOpaque.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	device->CreateBlendState(&blendDescOpaque, blendStateOpaque.GetAddressOf());

	// ダミーテクスチャ
	GpuResourceUtils::CreateDummyTexture(device, dummyWhite, 0xFFFFFFFF, 1);  // Albedo/Occlusion用
	GpuResourceUtils::CreateDummyTexture(device, dummyNormal, 0xFFFF8080, 1); // 法線
	GpuResourceUtils::CreateDummyTexture(device, dummyBlack, 0xFF000000, 1);  // Emissive用
}

// 描画
void RenderSystem::Render(
	entt::registry& registry,
	ID3D11DeviceContext* dc,
	ID3D11RenderTargetView* rtv,
	ID3D11DepthStencilView* dsv,
	ID3D11Device* device, 
	UINT width, 
	UINT height)
{
	// GBuffer初期化
	if (screenWidth != width || screenHeight != height || screenWidth == 0)
	{
		screenWidth = width;
		screenHeight = height;
		gBuffer.Initialize(device, width, height);
	}

	// 不透明ジオメトリをGBufferへ
	GeometryPass(registry, dc, dsv, cachedFrustum);

	// IBLテクスチャをLightingPassとForwardTransparentPassへ
	ID3D11ShaderResourceView* iblSrvs[3] = { iblDiffuseIem, iblSpecularPmrem, iblLutGgx };
	dc->PSSetShaderResources(6, 3, iblSrvs);

	// Lightingパス
	LightingPass(dc, rtv, dsv);

	// 半透明をフォワードで合成
	ForwardTransparentPass(registry, dc, rtv, dsv, cachedFrustum);
}

// フラスタム計算
RenderSystem::Frustum RenderSystem::CalcFrustum(
	const DirectX::XMFLOAT4X4& view,
	const DirectX::XMFLOAT4X4& projection)
{
	// ViewProjection行列を計算
	DirectX::XMMATRIX vp = DirectX::XMLoadFloat4x4(&view) * DirectX::XMLoadFloat4x4(&projection);
	DirectX::XMFLOAT4X4 m;
	DirectX::XMStoreFloat4x4(&m, vp);

	Frustum frustum;
	frustum.planes[0] = { { m._14 + m._11, m._24 + m._21, m._34 + m._31 }, m._44 + m._41 }; // Left
	frustum.planes[1] = { { m._14 - m._11, m._24 - m._21, m._34 - m._31 }, m._44 - m._41 }; // Right
	frustum.planes[2] = { { m._14 + m._12, m._24 + m._22, m._34 + m._32 }, m._44 + m._42 }; // Bottom
	frustum.planes[3] = { { m._14 - m._12, m._24 - m._22, m._34 - m._32 }, m._44 - m._42 }; // Top
	frustum.planes[4] = { { m._13,         m._23,         m._33         }, m._43         }; // Near
	frustum.planes[5] = { { m._14 - m._13, m._24 - m._23, m._34 - m._33 }, m._44 - m._43 }; // Far

	// 各平面を正規化
	for (auto& plane : frustum.planes)
	{
		float len = sqrtf(
			plane.normal.x * plane.normal.x +
			plane.normal.y * plane.normal.y +
			plane.normal.z * plane.normal.z
		);
		if (len > 0.0f)
		{
			plane.normal.x /= len;
			plane.normal.y /= len;
			plane.normal.z /= len;
			plane.distance /= len;
		}
	}
	return frustum;
}

// バウンディングボックスがフラスタム内にあるか判定
bool RenderSystem::IsInFrustum(const Frustum& frustum, const Model::BoundingBox& bbox, const DirectX::XMFLOAT4X4& world)
{
	// ワールド空間でのAABB8頂点を計算
	DirectX::XMMATRIX w = DirectX::XMLoadFloat4x4(&world);
	DirectX::XMVECTOR corners[8] =
	{
		DirectX::XMVector3TransformCoord(DirectX::XMVectorSet(bbox.min.x, bbox.min.y, bbox.min.z, 1), w),
		DirectX::XMVector3TransformCoord(DirectX::XMVectorSet(bbox.max.x, bbox.min.y, bbox.min.z, 1), w),
		DirectX::XMVector3TransformCoord(DirectX::XMVectorSet(bbox.min.x, bbox.max.y, bbox.min.z, 1), w),
		DirectX::XMVector3TransformCoord(DirectX::XMVectorSet(bbox.max.x, bbox.max.y, bbox.min.z, 1), w),
		DirectX::XMVector3TransformCoord(DirectX::XMVectorSet(bbox.min.x, bbox.min.y, bbox.max.z, 1), w),
		DirectX::XMVector3TransformCoord(DirectX::XMVectorSet(bbox.max.x, bbox.min.y, bbox.max.z, 1), w),
		DirectX::XMVector3TransformCoord(DirectX::XMVectorSet(bbox.min.x, bbox.max.y, bbox.max.z, 1), w),
		DirectX::XMVector3TransformCoord(DirectX::XMVectorSet(bbox.max.x, bbox.max.y, bbox.max.z, 1), w),
	};

	// 各平面に対して全頂点が外側にあればカリング
	for (const auto& plane : frustum.planes)
	{
		DirectX::XMVECTOR n = DirectX::XMVectorSet(plane.normal.x, plane.normal.y, plane.normal.z, 0);
		int outsideCount = 0;
		for (const auto& corner : corners)
		{
			if (DirectX::XMVectorGetX(DirectX::XMVector3Dot(n, corner)) + plane.distance < 0.0f)
			{
				outsideCount++;
			}
		}
		// 8頂点全て外側ならフラスタム外とする
		if (outsideCount == 8) 
		{
			return false;
		}
	}
	return true;
}

// Geometryパス
void RenderSystem::GeometryPass(
	entt::registry& registry,
	ID3D11DeviceContext* dc, 
	ID3D11DepthStencilView* dsv, 
	const Frustum& frustum)
{
	// GバッファのRTVをセット＋クリア
	gBuffer.BeginGeometryPass(dc, dsv);

	// ビューポート設定
	D3D11_VIEWPORT vp{};
	vp.Width = static_cast<float>(screenWidth);
	vp.Height = static_cast<float>(screenHeight);
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	dc->RSSetViewports(1, &vp);

	// 共通ステート設定
	dc->RSSetState(rasterizerState.Get());
	dc->PSSetSamplers(0, 1, samplerState.GetAddressOf());
	dc->IASetInputLayout(inputLayout.Get());
	dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// 不透明パス 
	float blendFactor[4] = {};
	dc->OMSetDepthStencilState(depthStencilStateGeometry.Get(), 1);
	dc->OMSetBlendState(blendStateOpaque.Get(), blendFactor, 0xFFFFFFFF);

	auto& defaultPS = gBufferPixelShaders.at("gbuffer_default");

	// 静的メッシュ(不透明）
	auto staticView = registry.view<TransformComponent, StaticMeshComponent>();
	staticView.each([&](auto entity, TransformComponent& transform, StaticMeshComponent& staticMesh)
		{
			if (!staticMesh.model) return;

			DirectX::XMFLOAT4X4 worldMatrix{};
			DirectX::XMStoreFloat4x4(&worldMatrix, transform.GetWorldMatrix());
			if (!IsInFrustum(frustum, staticMesh.model->GetBoundingBox(), worldMatrix)) return;

			auto* mat = registry.try_get<MaterialComponent>(entity);
			const char* shaderName = mat ? mat->GetShaderName() : "gbuffer_default";
			auto it = gBufferPixelShaders.find(shaderName);
			auto& ps = (it != gBufferPixelShaders.end()) ? it->second : defaultPS;
			dc->PSSetShader(ps.Get(), nullptr, 0);
			dc->VSSetShader(staticVertexShader.Get(), nullptr, 0);

			CbObject cbObjectData{};
			cbObjectData.world = worldMatrix;
			dc->UpdateSubresource(cbObject.Get(), 0, nullptr, &cbObjectData, 0, 0);
			dc->VSSetConstantBuffers(1, 1, cbObject.GetAddressOf());

			for (auto& mesh : staticMesh.model->GetMeshes())
			{
				// 不透明パスで描くのは Opaque・Mask
				bool isBlend = mesh.material &&
					mesh.material->alphaMode == Model::Material::AlphaMode::Blend;
				if (isBlend) continue;

				// Maskならクリップしきい値をセット、それ以外は破棄しない
				bool isMask = mesh.material &&
					mesh.material->alphaMode == Model::Material::AlphaMode::Mask;
				cbObjectData.alphaCutoff = isMask ? mesh.material->alphaCutoff : 0.0f;
				cbObjectData.baseColor = mesh.material
					? mesh.material->baseColor
					: DirectX::XMFLOAT4{ 1.0f, 1.0f, 1.0f, 1.0f };
				dc->UpdateSubresource(cbObject.Get(), 0, nullptr, &cbObjectData, 0, 0);
				dc->PSSetConstantBuffers(2, 1, cbObject.GetAddressOf());

				UINT stride = sizeof(Model::Vertex), offset = 0;
				dc->IASetVertexBuffers(0, 1, mesh.vertexBuffer.GetAddressOf(), &stride, &offset);
				dc->IASetIndexBuffer(mesh.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

				ID3D11ShaderResourceView* srvs[5] =
				{
					(mesh.material && mesh.material->baseMap) ? mesh.material->baseMap.Get() : dummyWhite.Get(),
					(mesh.material && mesh.material->normalMap) ? mesh.material->normalMap.Get() : dummyNormal.Get(),
					(mesh.material && mesh.material->metallicRoughnessMap) ? mesh.material->metallicRoughnessMap.Get() : dummyWhite.Get(),
					(mesh.material && mesh.material->occlusionMap) ? mesh.material->occlusionMap.Get() : dummyWhite.Get(),
					(mesh.material && mesh.material->emissiveMap) ? mesh.material->emissiveMap.Get() : dummyBlack.Get(),
				};
				dc->PSSetShaderResources(0, 5, srvs);
				dc->DrawIndexed(static_cast<UINT>(mesh.indices.size()), 0, 0);
			}
		});

	// スキニングメッシュ(不透明）
	auto skinnedView = registry.view<TransformComponent, SkinnedMeshComponent>();
	skinnedView.each([&](auto entity, TransformComponent& transform, SkinnedMeshComponent& skinnedMesh)
		{
			if (!skinnedMesh.model) return;

			DirectX::XMFLOAT4X4 worldMatrix{};
			DirectX::XMStoreFloat4x4(&worldMatrix, transform.GetWorldMatrix());
			if (!IsInFrustum(frustum, skinnedMesh.model->GetBoundingBox(), worldMatrix)) return;

			auto* mat = registry.try_get<MaterialComponent>(entity);
			const char* shaderName = mat ? mat->GetShaderName() : "gbuffer_default";
			auto it = gBufferPixelShaders.find(shaderName);
			auto& ps = (it != gBufferPixelShaders.end()) ? it->second : defaultPS;
			dc->PSSetShader(ps.Get(), nullptr, 0);
			dc->VSSetShader(skinnedVertexShader.Get(), nullptr, 0);

			CbObject cbObjectData{};
			cbObjectData.world = worldMatrix;
			dc->UpdateSubresource(cbObject.Get(), 0, nullptr, &cbObjectData, 0, 0);
			dc->VSSetConstantBuffers(1, 1, cbObject.GetAddressOf());

			const auto& meshes = skinnedMesh.model->GetMeshes();
			for (int m = 0; m < (int)meshes.size(); m++)
			{
				const auto& mesh = meshes[m];

				// 不透明パスで描くのは Opaque・Mask
				bool isBlend = mesh.material &&
					mesh.material->alphaMode == Model::Material::AlphaMode::Blend;
				if (isBlend) continue;

				// Maskならクリップしきい値をセット、それ以外は破棄しない
				bool isMask = mesh.material &&
					mesh.material->alphaMode == Model::Material::AlphaMode::Mask;
				cbObjectData.alphaCutoff = isMask ? mesh.material->alphaCutoff : 0.0f;
				cbObjectData.baseColor = mesh.material
					? mesh.material->baseColor
					: DirectX::XMFLOAT4{ 1.0f, 1.0f, 1.0f, 1.0f };
				dc->UpdateSubresource(cbObject.Get(), 0, nullptr, &cbObjectData, 0, 0);
				dc->PSSetConstantBuffers(2, 1, cbObject.GetAddressOf());

				CbSkinning cbSkinningData{};
				if (m < (int)skinnedMesh.boneTransforms.size() && !skinnedMesh.boneTransforms[m].empty())
				{
					const auto& bt = skinnedMesh.boneTransforms[m];
					size_t boneCount = (std::min)(bt.size(), (size_t)MAX_BONES);
					for (size_t i = 0; i < boneCount; i++)
						cbSkinningData.boneTransforms[i] = bt[i];
				}
				else
				{
					int nodeIndex = mesh.nodeIndex;
					if (nodeIndex >= 0 && nodeIndex < (int)skinnedMesh.nodeGlobalTransforms.size())
						cbSkinningData.boneTransforms[0] = skinnedMesh.nodeGlobalTransforms[nodeIndex];
					else
						cbSkinningData.boneTransforms[0] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
				}
				dc->UpdateSubresource(cbSkinning.Get(), 0, nullptr, &cbSkinningData, 0, 0);
				dc->VSSetConstantBuffers(2, 1, cbSkinning.GetAddressOf());

				UINT stride = sizeof(Model::Vertex), offset = 0;
				dc->IASetVertexBuffers(0, 1, mesh.vertexBuffer.GetAddressOf(), &stride, &offset);
				dc->IASetIndexBuffer(mesh.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

				ID3D11ShaderResourceView* srvs[5] =
				{
					(mesh.material && mesh.material->baseMap) ? mesh.material->baseMap.Get() : dummyWhite.Get(),
					(mesh.material && mesh.material->normalMap) ? mesh.material->normalMap.Get() : dummyNormal.Get(),
					(mesh.material && mesh.material->metallicRoughnessMap) ? mesh.material->metallicRoughnessMap.Get() : dummyWhite.Get(),
					(mesh.material && mesh.material->occlusionMap) ? mesh.material->occlusionMap.Get() : dummyWhite.Get(),
					(mesh.material && mesh.material->emissiveMap) ? mesh.material->emissiveMap.Get() : dummyBlack.Get(),
				};
				dc->PSSetShaderResources(0, 5, srvs);
				dc->DrawIndexed(static_cast<UINT>(mesh.indices.size()), 0, 0);
			}
		});

	gBuffer.EndGeometryPass(dc);
}

// Lightingパス
void RenderSystem::LightingPass(ID3D11DeviceContext* dc, ID3D11RenderTargetView* rtv, ID3D11DepthStencilView* dsv)
{
	// メインRTVにセット
	dc->OMSetRenderTargets(1, &rtv, dsv);

	// ビューポート設定
	D3D11_VIEWPORT vp{};
	vp.Width = static_cast<float>(screenWidth);
	vp.Height = static_cast<float>(screenHeight);
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	dc->RSSetViewports(1, &vp);

	// GバッファをSRVとしてセット
	ID3D11ShaderResourceView* gbSrvs[6] =
	{
		gBuffer.GetAlbedoSRV(),
		gBuffer.GetNormalSRV(),
		gBuffer.GetPositionSRV(),
		gBuffer.GetRoughnessMetallicSRV(),
		gBuffer.GetEmissiveSRV(),
		gBuffer.GetOcclusionSRV(),
	};
	dc->PSSetShaderResources(0, 6, gbSrvs);

	// Lightingシェーダーセット
	dc->VSSetShader(lightingVS.Get(), nullptr, 0);
	dc->PSSetShader(lightingPS.Get(), nullptr, 0);

	// 入力レイアウトなし
	dc->IASetInputLayout(nullptr);
	dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	// カリングなしラスタライザー
	dc->RSSetState(rasterizerStateNoCull.Get());

	// 深度書き込みOFF
	dc->OMSetDepthStencilState(depthStencilStateLighting.Get(), 1);

	// CbSceneをピクセルシェーダーにセット
	dc->PSSetConstantBuffers(0, 1, cbScene.GetAddressOf());

	// サンプラーもセット
	dc->PSSetSamplers(0, 1, samplerState.GetAddressOf());

	// フルスクリーン描画
	dc->Draw(4, 0);

	// GBufferSRVの束縛を解除
	ID3D11ShaderResourceView* nullSrvs[9] = {};
	dc->PSSetShaderResources(0, 9, nullSrvs);
}

// 半透明パス(フォワードレンダリング)
void RenderSystem::ForwardTransparentPass(
	entt::registry& registry,
	ID3D11DeviceContext* dc,
	ID3D11RenderTargetView* rtv,
	ID3D11DepthStencilView* dsv,
	const Frustum& frustum)
{
	dc->OMSetRenderTargets(1, &rtv, dsv);

	D3D11_VIEWPORT vp{};
	vp.Width = static_cast<float>(screenWidth);
	vp.Height = static_cast<float>(screenHeight);
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	dc->RSSetViewports(1, &vp);

	dc->RSSetState(rasterizerState.Get());
	dc->PSSetSamplers(0, 1, samplerState.GetAddressOf());
	dc->IASetInputLayout(inputLayout.Get());
	dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	float blendFactor[4] = {};
	dc->OMSetDepthStencilState(depthStencilStateTransparent.Get(), 0);
	dc->OMSetBlendState(blendState.Get(), blendFactor, 0xFFFFFFFF);

	// LightingPass末尾でSRVがクリアされているため改めてセット
	ID3D11ShaderResourceView* iblSrvs[3] = { iblDiffuseIem, iblSpecularPmrem, iblLutGgx };
	dc->PSSetShaderResources(5, 3, iblSrvs);

	dc->PSSetShader(forwardTransparentPS.Get(), nullptr, 0);

	// 静的メッシュ(半透明）
	auto staticView = registry.view<TransformComponent, StaticMeshComponent>();
	staticView.each([&](auto entity, TransformComponent& transform, StaticMeshComponent& staticMesh)
		{
			if (!staticMesh.model) return;

			DirectX::XMFLOAT4X4 worldMatrix{};
			DirectX::XMStoreFloat4x4(&worldMatrix, transform.GetWorldMatrix());
			if (!IsInFrustum(frustum, staticMesh.model->GetBoundingBox(), worldMatrix)) return;

			dc->VSSetShader(staticVertexShader.Get(), nullptr, 0);

			CbObject cbObjectData{};
			cbObjectData.world = worldMatrix;
			dc->UpdateSubresource(cbObject.Get(), 0, nullptr, &cbObjectData, 0, 0);
			dc->VSSetConstantBuffers(1, 1, cbObject.GetAddressOf());

			for (auto& mesh : staticMesh.model->GetMeshes())
			{
				// フォワードで描くのは Blend のみ
				bool isBlend = mesh.material &&
					mesh.material->alphaMode == Model::Material::AlphaMode::Blend;
				if (!isBlend) continue;

				// マテリアルの基本色をピクセルシェーダーへ
				cbObjectData.baseColor = mesh.material->baseColor;
				dc->UpdateSubresource(cbObject.Get(), 0, nullptr, &cbObjectData, 0, 0);
				dc->PSSetConstantBuffers(2, 1, cbObject.GetAddressOf());

				UINT stride = sizeof(Model::Vertex), offset = 0;
				dc->IASetVertexBuffers(0, 1, mesh.vertexBuffer.GetAddressOf(), &stride, &offset);
				dc->IASetIndexBuffer(mesh.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

				ID3D11ShaderResourceView* srvs[5] =
				{
					(mesh.material && mesh.material->baseMap) ? mesh.material->baseMap.Get() : dummyWhite.Get(),
					(mesh.material && mesh.material->normalMap) ? mesh.material->normalMap.Get() : dummyNormal.Get(),
					(mesh.material && mesh.material->metallicRoughnessMap) ? mesh.material->metallicRoughnessMap.Get() : dummyWhite.Get(),
					(mesh.material && mesh.material->occlusionMap) ? mesh.material->occlusionMap.Get() : dummyWhite.Get(),
					(mesh.material && mesh.material->emissiveMap) ? mesh.material->emissiveMap.Get() : dummyBlack.Get(),
				};
				dc->PSSetShaderResources(0, 5, srvs);
				dc->DrawIndexed(static_cast<UINT>(mesh.indices.size()), 0, 0);
			}
		});

	// スキニングメッシュ(半透明）
	auto skinnedView = registry.view<TransformComponent, SkinnedMeshComponent>();
	skinnedView.each([&](auto entity, TransformComponent& transform, SkinnedMeshComponent& skinnedMesh)
		{
			if (!skinnedMesh.model) return;

			DirectX::XMFLOAT4X4 worldMatrix{};
			DirectX::XMStoreFloat4x4(&worldMatrix, transform.GetWorldMatrix());
			if (!IsInFrustum(frustum, skinnedMesh.model->GetBoundingBox(), worldMatrix)) return;

			dc->VSSetShader(skinnedVertexShader.Get(), nullptr, 0);

			CbObject cbObjectData{};
			cbObjectData.world = worldMatrix;
			dc->UpdateSubresource(cbObject.Get(), 0, nullptr, &cbObjectData, 0, 0);
			dc->VSSetConstantBuffers(1, 1, cbObject.GetAddressOf());

			const auto& meshes = skinnedMesh.model->GetMeshes();
			for (int m = 0; m < (int)meshes.size(); m++)
			{
				const auto& mesh = meshes[m];

				// フォワードで描くのは Blend のみ
				bool isBlend = mesh.material &&
					mesh.material->alphaMode == Model::Material::AlphaMode::Blend;
				if (!isBlend) continue;

				CbSkinning cbSkinningData{};
				if (m < (int)skinnedMesh.boneTransforms.size() && !skinnedMesh.boneTransforms[m].empty())
				{
					const auto& bt = skinnedMesh.boneTransforms[m];
					size_t boneCount = (std::min)(bt.size(), (size_t)MAX_BONES);
					for (size_t i = 0; i < boneCount; i++)
						cbSkinningData.boneTransforms[i] = bt[i];
				}
				else
				{
					int nodeIndex = mesh.nodeIndex;
					if (nodeIndex >= 0 && nodeIndex < (int)skinnedMesh.nodeGlobalTransforms.size())
						cbSkinningData.boneTransforms[0] = skinnedMesh.nodeGlobalTransforms[nodeIndex];
					else
						cbSkinningData.boneTransforms[0] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
				}
				dc->UpdateSubresource(cbSkinning.Get(), 0, nullptr, &cbSkinningData, 0, 0);
				dc->VSSetConstantBuffers(2, 1, cbSkinning.GetAddressOf());

				UINT stride = sizeof(Model::Vertex), offset = 0;
				dc->IASetVertexBuffers(0, 1, mesh.vertexBuffer.GetAddressOf(), &stride, &offset);
				dc->IASetIndexBuffer(mesh.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

				ID3D11ShaderResourceView* srvs[5] =
				{
					(mesh.material && mesh.material->baseMap) ? mesh.material->baseMap.Get() : dummyWhite.Get(),
					(mesh.material && mesh.material->normalMap) ? mesh.material->normalMap.Get() : dummyNormal.Get(),
					(mesh.material && mesh.material->metallicRoughnessMap) ? mesh.material->metallicRoughnessMap.Get() : dummyWhite.Get(),
					(mesh.material && mesh.material->occlusionMap) ? mesh.material->occlusionMap.Get() : dummyWhite.Get(),
					(mesh.material && mesh.material->emissiveMap) ? mesh.material->emissiveMap.Get() : dummyBlack.Get(),
				};
				dc->PSSetShaderResources(0, 5, srvs);
				dc->DrawIndexed(static_cast<UINT>(mesh.indices.size()), 0, 0);
			}
		});

	// SRVの束縛解除（t0〜t7）
	ID3D11ShaderResourceView* nullSrvs[8] = {};
	dc->PSSetShaderResources(0, 8, nullSrvs);
}
