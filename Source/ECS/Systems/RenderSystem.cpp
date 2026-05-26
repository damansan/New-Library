#include "RenderSystem.h"
#include "Resource/GpuResourceUtils.h"
#include "Utility/misc.h"

// 初期化(シェーダー読み込み）
void RenderSystem::Initialize(ID3D11Device* device)
{
	// 入力レイアウト
	const std::vector<D3D11_INPUT_ELEMENT_DESC>& inputElementDescs = Model::InputElementDescs;

	// 頂点シェーダー読み込み
	GpuResourceUtils::LoadVertexShader(
		device,
		"Data/Shader/Base_VS.cso",
		inputElementDescs.data(),
		static_cast<UINT>(inputElementDescs.size()),
		inputLayout,
		vertexShader
	);

	// スキニング頂点シェーダー読み込み
	GpuResourceUtils::LoadVertexShader(
		device,
		"Data/Shader/Skinning_VS.cso",
		inputElementDescs.data(),
		static_cast<UINT>(inputElementDescs.size()),
		inputLayout,
		skinnedVertexShader
	);

	// ピクセルシェーダー読み込み(Basic)
	Microsoft::WRL::ComPtr<ID3D11PixelShader> basicPS;
	GpuResourceUtils::LoadPixelShader(
		device, "Data/Shader/Basic_PS.cso", basicPS);
	pixelShaders["basic"] = basicPS;

	// ピクセルシェーダー読み込み(Lambert)
	Microsoft::WRL::ComPtr<ID3D11PixelShader> lambertPS;
	GpuResourceUtils::LoadPixelShader(
		device, "Data/Shader/Lambert_PS.cso", lambertPS);
	pixelShaders["lambert"] = lambertPS;


	pixelShaders["skinning_basic"] = basicPS;
	pixelShaders["skinning_lambert"] = lambertPS;

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
}

// 描画
void RenderSystem::Render(entt::registry& registry, ID3D11DeviceContext* dc, ID3D11RenderTargetView* rtv, ID3D11DepthStencilView* dsv)
{
	// レンダーターゲット再セット
	dc->OMSetRenderTargets(1, &rtv, dsv);

	// ステート設定
	dc->RSSetState(rasterizerState.Get());
	dc->OMSetDepthStencilState(depthStencilState.Get(), 0);
	dc->PSSetSamplers(0, 1, samplerState.GetAddressOf());

	// シェーダー設定
	dc->PSSetSamplers(0, 1, samplerState.GetAddressOf());
	dc->VSSetShader(vertexShader.Get(), nullptr, 0);
	dc->IASetInputLayout(inputLayout.Get());
	dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


	// 静的メッシュ描画
	for (auto& [passName, ps] : pixelShaders)
	{
		dc->PSSetShader(ps.Get(), nullptr, 0);
		// 静的メッシュ描画
		RenderStatic(registry, dc, passName);
	}

	// スキニングメッシュ描画
	for (auto& [passName, ps] : pixelShaders)
	{
		dc->VSSetShader(skinnedVertexShader.Get(), nullptr, 0);
		dc->PSSetShader(ps.Get(), nullptr, 0);
		RenderSkinned(registry, dc, passName);
	}
}

// 静的メッシュ描画
void RenderSystem::RenderStatic(
	entt::registry& registry,
	ID3D11DeviceContext* dc, 
	const std::string& passName)
{
	// TransformComponent + StaticMeshComponent を持つEntityを取得
	auto view = registry.view<TransformComponent, StaticMeshComponent>();

	view.each([&](auto entity, TransformComponent& transform, StaticMeshComponent& staticMesh)
		{
			// モデルが未設定ならスキップ
			if (!staticMesh.model) return;


			// MaterialComponentがある場合はパスを確認
			if (registry.all_of<MaterialComponent>(entity))
			{
				auto& mat = registry.get<MaterialComponent>(entity);
				bool hasPass = false;
				for (auto& pass : mat.passes)
				{
					if (pass.name == passName && pass.enabled)
					{
						hasPass = true;
						break;
					}
				}
				// 指定パスがなければスキップ
				if (!hasPass) return;
			}
			else
			{
				// MaterialComponentがない場合はbasicのみ
				if (passName != "basic") return;
			}

			// ワールド行列を定数バッファに設定
			CbObject cbObjectData{};
			DirectX::XMStoreFloat4x4(
				&cbObjectData.world,
				transform.GetWorldMatrix()
			);

			dc->UpdateSubresource(cbObject.Get(), 0, nullptr, &cbObjectData, 0, 0);
			dc->VSSetConstantBuffers(1, 1, cbObject.GetAddressOf());

			// メッシュ単位で描画
			for (auto& mesh : staticMesh.model->GetMeshes())
			{
				// 頂点バッファ設定
				UINT stride = sizeof(Model::Vertex);
				UINT offset = 0;
				dc->IASetVertexBuffers(
					0, 1,
					mesh.vertexBuffer.GetAddressOf(),
					&stride, &offset
				);

				// インデックスバッファ設定
				dc->IASetIndexBuffer(
					mesh.indexBuffer.Get(),
					DXGI_FORMAT_R32_UINT, 0
				);

				// ベースカラーテクスチャ設定
				if (mesh.material && mesh.material->baseMap)
				{
					dc->PSSetShaderResources(
						0, 1,
						mesh.material->baseMap.GetAddressOf()
					);
				}

				// 描画
				dc->DrawIndexed(
					static_cast<UINT>(mesh.indices.size()),
					0, 0
				);
			}

		});
}

// スキニングメッシュ描画
void RenderSystem::RenderSkinned(entt::registry& registry, ID3D11DeviceContext* dc, const std::string& passName)
{
	auto view = registry.view<TransformComponent, SkinnedMeshComponent>();

	view.each([&](auto entity,
		TransformComponent& transform,
		SkinnedMeshComponent& skinnedMesh)
		{
			if (!skinnedMesh.model) return;

			// パス確認
			if (registry.all_of<MaterialComponent>(entity))
			{
				auto& mat = registry.get<MaterialComponent>(entity);
				bool hasPass = false;
				for (auto& pass : mat.passes)
				{
					if (pass.name == passName && pass.enabled)
					{
						hasPass = true;
						break;
					}
				}
				if (!hasPass) return;
			}
			else
			{
				if (passName != "skinning_lambert") return;
			}

			// ワールド行列更新
			CbObject cbObjectData{};
			DirectX::XMStoreFloat4x4(
				&cbObjectData.world,
				transform.GetWorldMatrix()
			);
			dc->UpdateSubresource(cbObject.Get(), 0, nullptr, &cbObjectData, 0, 0);
			dc->VSSetConstantBuffers(1, 1, cbObject.GetAddressOf());

			// ボーン行列更新
			CbSkinning cbSkinningData{};
			const auto& boneTransforms = skinnedMesh.boneTransforms;
			size_t boneCount = (std::min)(boneTransforms.size(), (size_t)MAX_BONES);
			for (size_t i = 0; i < boneCount; i++)
			{
				cbSkinningData.boneTransforms[i] = boneTransforms[i];
			}
			dc->UpdateSubresource(cbSkinning.Get(), 0, nullptr, &cbSkinningData, 0, 0);
			dc->VSSetConstantBuffers(2, 1, cbSkinning.GetAddressOf());

			// メッシュ描画
			for (auto& mesh : skinnedMesh.model->GetMeshes())
			{
				UINT stride = sizeof(Model::Vertex);
				UINT offset = 0;
				dc->IASetVertexBuffers(0, 1, mesh.vertexBuffer.GetAddressOf(), &stride, &offset);
				dc->IASetIndexBuffer(mesh.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

				if (mesh.material && mesh.material->baseMap)
				{
					dc->PSSetShaderResources(0, 1, mesh.material->baseMap.GetAddressOf());
				}

				dc->DrawIndexed(static_cast<UINT>(mesh.indices.size()), 0, 0);
			}
		});
}
