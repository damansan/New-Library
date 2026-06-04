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
	Microsoft::WRL::ComPtr<ID3D11InputLayout> dummyLayout;
	GpuResourceUtils::LoadVertexShader(
		device,
		"Data/Shader/Skinning_VS.cso",
		inputElementDescs.data(),
		static_cast<UINT>(inputElementDescs.size()),
		dummyLayout,
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
}

// 描画
void RenderSystem::Render(entt::registry& registry, ID3D11DeviceContext* dc, ID3D11RenderTargetView* rtv, ID3D11DepthStencilView* dsv)
{
	// レンダーターゲット再セット
	dc->OMSetRenderTargets(1, &rtv, dsv);

	// 共通ステート設定
	dc->RSSetState(rasterizerState.Get());
	dc->PSSetSamplers(0, 1, samplerState.GetAddressOf());
	dc->IASetInputLayout(inputLayout.Get());
	dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	// 不透明パス
	dc->OMSetDepthStencilState(depthStencilState.Get(), 0);
	dc->OMSetBlendState(blendState.Get(), blendFactor, 0xFFFFFFFF);

	// 静的メッシュ描画
	dc->VSSetShader(vertexShader.Get(), nullptr, 0);
	for (auto& [passName, ps] : pixelShaders)
	{
		dc->PSSetShader(ps.Get(), nullptr, 0);
		RenderStatic(registry, dc, passName, false);
	}

	// スキニングメッシュ描画
	dc->VSSetShader(skinnedVertexShader.Get(), nullptr, 0);
	for (auto& [passName, ps] : pixelShaders)
	{
		dc->PSSetShader(ps.Get(), nullptr, 0);
		RenderSkinned(registry, dc, passName, false);
	}

	// 半透明パス
	dc->OMSetDepthStencilState(depthStencilStateTransparent.Get(), 0);
	dc->OMSetBlendState(blendState.Get(), blendFactor, 0xFFFFFFFF);

	// 静的メッシュ描画
	dc->VSSetShader(vertexShader.Get(), nullptr, 0);
	for (auto& [passName, ps] : pixelShaders)
	{
		dc->PSSetShader(ps.Get(), nullptr, 0);
		RenderStatic(registry, dc, passName, true);
	}

	// スキニングメッシュ描画
	dc->VSSetShader(skinnedVertexShader.Get(), nullptr, 0);
	for (auto& [passName, ps] : pixelShaders)
	{
		dc->PSSetShader(ps.Get(), nullptr, 0);
		RenderSkinned(registry, dc, passName, true);
	}
}

// 静的メッシュ描画
void RenderSystem::RenderStatic(
	entt::registry& registry,
	ID3D11DeviceContext* dc, 
	const std::string& passName,
	bool transparent)
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
				// 透明フラグとマテリアルのalphaModeが一致するメッシュだけ描画
				if (mesh.material)
				{
					bool isTransparent =
						mesh.material->alphaMode == Model::Material::AlphaMode::Blend ||
						mesh.material->alphaMode == Model::Material::AlphaMode::Mask;
					if (isTransparent != transparent) continue;
				}
				else
				{
					if (transparent) continue;
				}

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
void RenderSystem::RenderSkinned(
	entt::registry& registry, 
	ID3D11DeviceContext* dc,
	const std::string& passName,
	bool transparent)
{
	auto view = registry.view<TransformComponent, SkinnedMeshComponent>();

	view.each([&](auto entity,
		TransformComponent& transform,
		SkinnedMeshComponent& skinnedMesh)
		{
			// モデルが未設定ならスキップ
			if (!skinnedMesh.model) return;

			// パス確認
			if (registry.all_of<MaterialComponent>(entity))
			{
				// MaterialComponentがある場合はパスを確認
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
				if (passName != "skinning_basic") return;
			}

			// ワールド行列を定数バッファに設定
			CbObject cbObjectData{};
			DirectX::XMStoreFloat4x4(
				&cbObjectData.world,
				transform.GetWorldMatrix()
			);
			dc->UpdateSubresource(cbObject.Get(), 0, nullptr, &cbObjectData, 0, 0);
			dc->VSSetConstantBuffers(1, 1, cbObject.GetAddressOf());

			// メッシュ描画（メッシュごとに対応するボーン行列を転送）
			const auto& meshes = skinnedMesh.model->GetMeshes();
			for (int m = 0; m < (int)meshes.size(); m++)
			{
				const auto& mesh = meshes[m];

				// 透明フラグとマテリアルのalphaModeが一致するメッシュだけ描画
				if (mesh.material)
				{
					bool isTransparent =
						mesh.material->alphaMode == Model::Material::AlphaMode::Blend ||
						mesh.material->alphaMode == Model::Material::AlphaMode::Mask;
					if (isTransparent != transparent) continue;
				}
				else
				{
					if (transparent) continue;
				}

				//CbSkinning cbSkinningData{};
				//if (m < (int)skinnedMesh.boneTransforms.size())
				//{
				//	const auto& bt = skinnedMesh.boneTransforms[m];
				//	size_t boneCount = (std::min)(bt.size(), (size_t)MAX_BONES);
				//	for (size_t i = 0; i < boneCount; i++)
				//	{
				//		cbSkinningData.boneTransforms[i] = bt[i];
				//	}
				//}
				//dc->UpdateSubresource(cbSkinning.Get(), 0, nullptr, &cbSkinningData, 0, 0);
				//dc->VSSetConstantBuffers(2, 1, cbSkinning.GetAddressOf());
				CbSkinning cbSkinningData{};
				if (m < (int)skinnedMesh.boneTransforms.size() &&
					!skinnedMesh.boneTransforms[m].empty())
				{
					const auto& bt = skinnedMesh.boneTransforms[m];
					size_t boneCount = (std::min)(bt.size(), (size_t)MAX_BONES);
					for (size_t i = 0; i < boneCount; i++)
					{
						cbSkinningData.boneTransforms[i] = bt[i];
					}
				}
				else
				{
					// bones=0 のメッシュはノードのグローバルトランスフォームを使う
					const auto& nodes = skinnedMesh.model->GetNodes();
					int nodeIndex = mesh.nodeIndex;
					if (nodeIndex >= 0 && nodeIndex < (int)nodes.size())
					{
						cbSkinningData.boneTransforms[0] =
							nodes[nodeIndex].globalTransform;
					}
					else
					{
						cbSkinningData.boneTransforms[0] =
						{ 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
					}
				}
				dc->UpdateSubresource(cbSkinning.Get(), 0, nullptr, &cbSkinningData, 0, 0);
				dc->VSSetConstantBuffers(2, 1, cbSkinning.GetAddressOf());



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
