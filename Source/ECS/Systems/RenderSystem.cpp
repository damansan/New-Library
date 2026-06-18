#include "RenderSystem.h"
#include "Resource/GpuResourceUtils.h"
#include "Utility/misc.h"

// 初期化(シェーダー読み込み）
void RenderSystem::Initialize(ID3D11Device* device)
{
	// 入力レイアウト
	const std::vector<D3D11_INPUT_ELEMENT_DESC>& inputElementDescs = Model::InputElementDescs;

	// 静的メッシュ用頂点シェーダー
	GpuResourceUtils::LoadVertexShader(
		device,
		"Data/Shader/Static_VS.cso",
		inputElementDescs.data(),
		static_cast<UINT>(inputElementDescs.size()),
		inputLayout,
		vertexShader
	);

	// スキニング用頂点シェーダー
	Microsoft::WRL::ComPtr<ID3D11InputLayout> dummyLayout;
	GpuResourceUtils::LoadVertexShader(
		device,
		"Data/Shader/Skinning_VS.cso",
		inputElementDescs.data(),
		static_cast<UINT>(inputElementDescs.size()),
		dummyLayout,
		skinnedVertexShader
	);

	// GBufferシェーダーをマップに登録
	Microsoft::WRL::ComPtr<ID3D11PixelShader> psDefault;
	GpuResourceUtils::LoadPixelShader(device, "Data/Shader/Gbuffer_PS.cso", psDefault);
	gBufferPixelShaders["gbuffer_default"] = psDefault;

	Microsoft::WRL::ComPtr<ID3D11PixelShader> psBasic;
	GpuResourceUtils::LoadPixelShader(device, "Data/Shader/Gbuffer_Basic_PS.cso", psBasic);
	gBufferPixelShaders["gbuffer_basic"] = psBasic;

	// シーン定数バッファ生成
	GpuResourceUtils::CreateConstantBuffer(
		device,
		sizeof(CbScene),
		cbScene
	);

	// オブジェクト定数バッファ生成
	GpuResourceUtils::CreateConstantBuffer(
		device,
		sizeof(CbObject),
		cbObject
	);

	// スキニング定数バッファ生成
	GpuResourceUtils::CreateConstantBuffer(
		device,
		sizeof(CbSkinning),
		cbSkinning
	);

	// サンプラーステート生成
	D3D11_SAMPLER_DESC samplerDesc{};
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR; // 線形補間
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;    // UV繰り返し
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	device->CreateSamplerState(
		&samplerDesc,
		samplerState.GetAddressOf()
	);

	// ラスタライザーステート生成
	D3D11_RASTERIZER_DESC rasterizerDesc{};
	rasterizerDesc.FillMode = D3D11_FILL_SOLID; // 塗りつぶし
	rasterizerDesc.CullMode = D3D11_CULL_BACK;  // 裏面カリング
	rasterizerDesc.FrontCounterClockwise = FALSE;
	device->CreateRasterizerState(
		&rasterizerDesc,
		rasterizerState.GetAddressOf()
	);

	// 深度ステンシルステート生成
	D3D11_DEPTH_STENCIL_DESC depthStencilDesc{};
	depthStencilDesc.DepthEnable = TRUE;                          // 深度テスト有効
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL; // 深度書き込み有効
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;     // 以下なら描画
	device->CreateDepthStencilState(
		&depthStencilDesc,
		depthStencilState.GetAddressOf()
	);

	// 半透明用深度ステンシルステート（深度テストON、深度書き込みOFF）
	D3D11_DEPTH_STENCIL_DESC depthStencilDescTransparent{};
	depthStencilDescTransparent.DepthEnable = TRUE;
	depthStencilDescTransparent.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO; // 書き込みOFF
	depthStencilDescTransparent.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	device->CreateDepthStencilState(
		&depthStencilDescTransparent,
		depthStencilStateTransparent.GetAddressOf()
	);

	// ブレンドステート生成（アルファブレンド）
	D3D11_BLEND_DESC blendDesc{};
	blendDesc.AlphaToCoverageEnable = FALSE;
	blendDesc.IndependentBlendEnable = FALSE;
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	device->CreateBlendState(&blendDesc, blendState.GetAddressOf());
	// GBuffer用（ブレンドなし）
	D3D11_BLEND_DESC blendDescOpaque{};
	blendDescOpaque.AlphaToCoverageEnable = FALSE;
	blendDescOpaque.IndependentBlendEnable = FALSE;
	blendDescOpaque.RenderTarget[0].BlendEnable = FALSE;
	blendDescOpaque.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	device->CreateBlendState(&blendDescOpaque, blendStateOpaque.GetAddressOf());

	// ダミーテクスチャ生成
	// 白 (Albedo / Occlusion用)
	GpuResourceUtils::CreateDummyTexture(device, dummyWhite, 0xFFFFFFFF, 1);
	// 法線マップ用 (R=128, G=128, B=255 → 正面向き法線)
	GpuResourceUtils::CreateDummyTexture(device, dummyNormal, 0xFFFF8080, 1);
	// 黒 (Emissive用)
	GpuResourceUtils::CreateDummyTexture(device, dummyBlack, 0xFF000000, 1);

	// Lightingパス用シェーダー読み込み
	Microsoft::WRL::ComPtr<ID3D11InputLayout> dummyLayoutL;
	GpuResourceUtils::LoadVertexShader(
		device,
		"Data/Shader/Lighting_VS.cso",
		nullptr, 0,
		dummyLayoutL,
		lightingVS
	);
	GpuResourceUtils::LoadPixelShader(
		device, "Data/Shader/Lighting_PS.cso", lightingPS);

	// ForwardTransparentパス用ピクセルシェーダー読み込み
	GpuResourceUtils::LoadPixelShader(
		device, "Data/Shader/PBR_PS.cso", forwardTransparentPS);

	// カリングなしラスタライザー
	D3D11_RASTERIZER_DESC noCullDesc{};
	noCullDesc.FillMode = D3D11_FILL_SOLID;
	noCullDesc.CullMode = D3D11_CULL_NONE;
	noCullDesc.FrontCounterClockwise = FALSE;
	device->CreateRasterizerState(
		&noCullDesc, rasterizerStateNoCull.GetAddressOf());
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

	Frustum frustum = cachedFrustum;

	// Geometryパス
	GeometryPass(registry, dc, dsv, frustum);

	// IBL用テクスチャセット
	ID3D11ShaderResourceView* iblSrvs[3] =
	{
		iblDiffuseIem,
		iblSpecularPmrem,
		iblLutGgx,
	};
	dc->PSSetShaderResources(6, 3, iblSrvs);

	// Lightingパス
	LightingPass(dc, rtv, dsv);

	// ForwardTransparentパス（半透明をフォワードで描画）
	ForwardTransparentPass(registry, dc, rtv, dsv, frustum);
}

// フラスタム計算
RenderSystem::Frustum RenderSystem::CalcFrustum(
	const DirectX::XMFLOAT4X4& view,
	const DirectX::XMFLOAT4X4& projection)
{
	// ViewProjection行列を計算
	DirectX::XMMATRIX v = DirectX::XMLoadFloat4x4(&view);
	DirectX::XMMATRIX p = DirectX::XMLoadFloat4x4(&projection);
	DirectX::XMMATRIX vp = v * p;

	DirectX::XMFLOAT4X4 m;
	DirectX::XMStoreFloat4x4(&m, vp);

	Frustum frustum;

	// Left
	frustum.planes[0].normal = { m._14 + m._11, m._24 + m._21, m._34 + m._31 };
	frustum.planes[0].distance = m._44 + m._41;
	// Right
	frustum.planes[1].normal = { m._14 - m._11, m._24 - m._21, m._34 - m._31 };
	frustum.planes[1].distance = m._44 - m._41;
	// Bottom
	frustum.planes[2].normal = { m._14 + m._12, m._24 + m._22, m._34 + m._32 };
	frustum.planes[2].distance = m._44 + m._42;
	// Top
	frustum.planes[3].normal = { m._14 - m._12, m._24 - m._22, m._34 - m._32 };
	frustum.planes[3].distance = m._44 - m._42;
	// Near
	frustum.planes[4].normal = { m._13, m._23, m._33 };
	frustum.planes[4].distance = m._43;
	// Far
	frustum.planes[5].normal = { m._14 - m._13, m._24 - m._23, m._34 - m._33 };
	frustum.planes[5].distance = m._44 - m._43;

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
bool RenderSystem::IsInFrustum(
	const Frustum& frustum,
	const Model::BoundingBox& bbox,
	const DirectX::XMFLOAT4X4& world)
{
	// ワールド空間でのAABB8頂点を計算
	DirectX::XMVECTOR corners[8];
	DirectX::XMMATRIX w = DirectX::XMLoadFloat4x4(&world);

	corners[0] = DirectX::XMVector3TransformCoord(
		DirectX::XMVectorSet(bbox.min.x, bbox.min.y, bbox.min.z, 1), w);
	corners[1] = DirectX::XMVector3TransformCoord(
		DirectX::XMVectorSet(bbox.max.x, bbox.min.y, bbox.min.z, 1), w);
	corners[2] = DirectX::XMVector3TransformCoord(
		DirectX::XMVectorSet(bbox.min.x, bbox.max.y, bbox.min.z, 1), w);
	corners[3] = DirectX::XMVector3TransformCoord(
		DirectX::XMVectorSet(bbox.max.x, bbox.max.y, bbox.min.z, 1), w);
	corners[4] = DirectX::XMVector3TransformCoord(
		DirectX::XMVectorSet(bbox.min.x, bbox.min.y, bbox.max.z, 1), w);
	corners[5] = DirectX::XMVector3TransformCoord(
		DirectX::XMVectorSet(bbox.max.x, bbox.min.y, bbox.max.z, 1), w);
	corners[6] = DirectX::XMVector3TransformCoord(
		DirectX::XMVectorSet(bbox.min.x, bbox.max.y, bbox.max.z, 1), w);
	corners[7] = DirectX::XMVector3TransformCoord(
		DirectX::XMVectorSet(bbox.max.x, bbox.max.y, bbox.max.z, 1), w);

	// 各平面に対して全頂点が外側にあればカリング
	for (const auto& plane : frustum.planes)
	{
		DirectX::XMVECTOR n = DirectX::XMVectorSet(
			plane.normal.x, plane.normal.y, plane.normal.z, 0);

		int outsideCount = 0;
		for (const auto& corner : corners)
		{
			float dot = DirectX::XMVectorGetX(
				DirectX::XMVector3Dot(n, corner));
			if (dot + plane.distance < 0.0f)
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

	float blendFactor[4] = {};

	// 不透明パス
	dc->OMSetDepthStencilState(depthStencilState.Get(), 0);
	dc->OMSetBlendState(blendStateOpaque.Get(), blendFactor, 0xFFFFFFFF);

	//// 静的メッシュ描画
	//dc->VSSetShader(vertexShader.Get(), nullptr, 0);
	//RenderStaticGeometry(registry, dc, false, frustum);

	//// スキニングメッシュ描画
	//dc->VSSetShader(skinnedVertexShader.Get(), nullptr, 0);
	//RenderSkinnedGeometry(registry, dc, false, frustum);

	 // デフォルトシェーダーを取得
	auto& defaultPS = gBufferPixelShaders.at("gbuffer_default");

	// エンティティごとにシェーダーを切り替えながら静的メッシュ描画
	auto staticView = registry.view<TransformComponent, StaticMeshComponent>();
	staticView.each([&](auto entity,
		TransformComponent& transform,
		StaticMeshComponent& staticMesh)
		{
			if (!staticMesh.model) return;

			DirectX::XMFLOAT4X4 worldMatrix{};
			DirectX::XMStoreFloat4x4(&worldMatrix, transform.GetWorldMatrix());
			if (!IsInFrustum(frustum, staticMesh.model->GetBoundingBox(), worldMatrix)) return;

			// MaterialComponentがあればそのシェーダーを使う、なければデフォルト
			auto* mat = registry.try_get<MaterialComponent>(entity);
			const char* shaderName = mat ? mat->GetShaderName() : "gbuffer_default";

			auto it = gBufferPixelShaders.find(shaderName);
			auto& ps = (it != gBufferPixelShaders.end()) ? it->second : defaultPS;
			dc->PSSetShader(ps.Get(), nullptr, 0);

			dc->VSSetShader(vertexShader.Get(), nullptr, 0);

			CbObject cbObjectData{};
			cbObjectData.world = worldMatrix;
			dc->UpdateSubresource(cbObject.Get(), 0, nullptr, &cbObjectData, 0, 0);
			dc->VSSetConstantBuffers(1, 1, cbObject.GetAddressOf());

			for (auto& mesh : staticMesh.model->GetMeshes())
			{
				if (mesh.material)
				{
					bool isTransparent =
						mesh.material->alphaMode == Model::Material::AlphaMode::Blend ||
						mesh.material->alphaMode == Model::Material::AlphaMode::Mask;
					if (isTransparent) continue;
				}

				UINT stride = sizeof(Model::Vertex);
				UINT offset = 0;
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
			//RenderMeshes(staticMesh.model->GetMeshes(), dc, false, nullptr, 0);
		});

	// エンティティごとにシェーダーを切り替えながらスキニングメッシュ描画
	auto skinnedView = registry.view<TransformComponent, SkinnedMeshComponent>();
	skinnedView.each([&](auto entity,
		TransformComponent& transform,
		SkinnedMeshComponent& skinnedMesh)
		{
			if (!skinnedMesh.model) return;

			DirectX::XMFLOAT4X4 worldMatrix{};
			DirectX::XMStoreFloat4x4(&worldMatrix, transform.GetWorldMatrix());
			if (!IsInFrustum(frustum, skinnedMesh.model->GetBoundingBox(), worldMatrix)) return;

			// MaterialComponentがあればそのシェーダーを使う、なければデフォルト
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

				if (mesh.material)
				{
					bool isTransparent =
						mesh.material->alphaMode == Model::Material::AlphaMode::Blend ||
						mesh.material->alphaMode == Model::Material::AlphaMode::Mask;
					if (isTransparent) continue;
				}

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
					const auto& nodes = skinnedMesh.model->GetNodes();
					int nodeIndex = mesh.nodeIndex;
					if (nodeIndex >= 0 && nodeIndex < (int)nodes.size())
						cbSkinningData.boneTransforms[0] = nodes[nodeIndex].globalTransform;
					else
						cbSkinningData.boneTransforms[0] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
				}
				dc->UpdateSubresource(cbSkinning.Get(), 0, nullptr, &cbSkinningData, 0, 0);
				dc->VSSetConstantBuffers(2, 1, cbSkinning.GetAddressOf());

				UINT stride = sizeof(Model::Vertex);
				UINT offset = 0;
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
			//RenderSkinnedMeshes(skinnedMesh, dc, false);
		});

	// Geometryパス終了
	gBuffer.EndGeometryPass(dc);
}

// Lightingパス
void RenderSystem::LightingPass(
	ID3D11DeviceContext* dc,
	ID3D11RenderTargetView* rtv,
	ID3D11DepthStencilView* dsv)
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
	dc->IASetPrimitiveTopology(
		D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	// カリングなしラスタライザー
	dc->RSSetState(rasterizerStateNoCull.Get());

	// 深度書き込みOFF
	dc->OMSetDepthStencilState(
		depthStencilStateTransparent.Get(), 0);

	// CbSceneをピクセルシェーダーにセット
	dc->PSSetConstantBuffers(0, 1, cbScene.GetAddressOf());
	// サンプラーもセット
	dc->PSSetSamplers(0, 1, samplerState.GetAddressOf());

	// フルスクリーン描画
	dc->Draw(4, 0);

	// SRVの束縛を解除
	ID3D11ShaderResourceView* nullSrvs[9] = {};
	dc->PSSetShaderResources(0, 9, nullSrvs);
}

// ForwardTransparentパス
void RenderSystem::ForwardTransparentPass(
	entt::registry& registry, 
	ID3D11DeviceContext* dc,
	ID3D11RenderTargetView* rtv, 
	ID3D11DepthStencilView* dsv,
	const Frustum& frustum)
{
	// メインRTV＋深度バッファに直接セット（GBufferを経由しない）
	dc->OMSetRenderTargets(1, &rtv, dsv);

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

	// 深度テストON・書き込みOFF、アルファブレンドON
	dc->OMSetDepthStencilState(depthStencilStateTransparent.Get(), 0);
	float blendFactor[4] = {};
	dc->OMSetBlendState(blendState.Get(), blendFactor, 0xFFFFFFFF);

	// IBL用テクスチャを再セット
	// （LightingPassの末尾でt0〜t8がnullクリアされているため、ここで改めてセットする）
	ID3D11ShaderResourceView* iblSrvs[3] =
	{
		iblDiffuseIem,
		iblSpecularPmrem,
		iblLutGgx,
	};
	dc->PSSetShaderResources(5, 3, iblSrvs);

	// フォワードPBR用ピクセルシェーダーを固定でセット
	dc->PSSetShader(forwardTransparentPS.Get(), nullptr, 0);

	// 静的メッシュ（半透明のみ）
	auto staticView = registry.view<TransformComponent, StaticMeshComponent>();
	staticView.each([&](auto entity,
		TransformComponent& transform,
		StaticMeshComponent& staticMesh)
		{
			if (!staticMesh.model) return;

			DirectX::XMFLOAT4X4 worldMatrix{};
			DirectX::XMStoreFloat4x4(&worldMatrix, transform.GetWorldMatrix());
			if (!IsInFrustum(frustum, staticMesh.model->GetBoundingBox(), worldMatrix)) return;

			dc->VSSetShader(vertexShader.Get(), nullptr, 0);

			CbObject cbObjectData{};
			cbObjectData.world = worldMatrix;
			dc->UpdateSubresource(cbObject.Get(), 0, nullptr, &cbObjectData, 0, 0);
			dc->VSSetConstantBuffers(1, 1, cbObject.GetAddressOf());

			for (auto& mesh : staticMesh.model->GetMeshes())
			{
				// 半透明のメッシュだけを描画する（不透明はGeometryPassで描画済み）
				bool isTransparent = mesh.material &&
					(mesh.material->alphaMode == Model::Material::AlphaMode::Blend ||
						mesh.material->alphaMode == Model::Material::AlphaMode::Mask);
				if (!isTransparent) continue;

				UINT stride = sizeof(Model::Vertex);
				UINT offset = 0;
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

	// スキニングメッシュ（半透明のみ）
	auto skinnedView = registry.view<TransformComponent, SkinnedMeshComponent>();
	skinnedView.each([&](auto entity,
		TransformComponent& transform,
		SkinnedMeshComponent& skinnedMesh)
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

				bool isTransparent = mesh.material &&
					(mesh.material->alphaMode == Model::Material::AlphaMode::Blend ||
						mesh.material->alphaMode == Model::Material::AlphaMode::Mask);
				if (!isTransparent) continue;

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
					const auto& nodes = skinnedMesh.model->GetNodes();
					int nodeIndex = mesh.nodeIndex;
					if (nodeIndex >= 0 && nodeIndex < (int)nodes.size())
						cbSkinningData.boneTransforms[0] = nodes[nodeIndex].globalTransform;
					else
						cbSkinningData.boneTransforms[0] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
				}
				dc->UpdateSubresource(cbSkinning.Get(), 0, nullptr, &cbSkinningData, 0, 0);
				dc->VSSetConstantBuffers(2, 1, cbSkinning.GetAddressOf());

				UINT stride = sizeof(Model::Vertex);
				UINT offset = 0;
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

	// SRVの束縛を解除（マテリアル5枚＋IBL3枚＝t0〜t7）
	ID3D11ShaderResourceView* nullSrvs[8] = {};
	dc->PSSetShaderResources(0, 8, nullSrvs);
}
