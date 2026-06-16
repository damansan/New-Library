#include "Tool/Studio/StudioWindow.h"
#include <imgui.h>
#include <cassert>

void StudioWindow::Initialize(ID3D11Device* device, StudioSharedContext* shared, UINT width, UINT height)
{
	renderWidth = width;
	renderHeight = height;
	context.device = device;
	this->shared = shared;

	renderSystem.Initialize(device);
	lightManager.Initialize(device);

	// IBLテクスチャセット
	renderSystem.SetIBLTextures(
		shared->skyBox.GetDiffuseIem(),
		shared->skyBox.GetSRV(),
		shared->skyBox.GetLutGgx()
	);

	modelViewer.Initialize(device, &shared->resourceManager);

	ResizeRenderTarget();

	//HRESULT hr;

	//// カラーテクスチャ生成
	//{
	//	D3D11_TEXTURE2D_DESC desc{};
	//	desc.Width = renderWidth;
	//	desc.Height = renderHeight;
	//	desc.MipLevels = 1;
	//	desc.ArraySize = 1;
	//	desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	//	desc.SampleDesc.Count = 1;
	//	desc.Usage = D3D11_USAGE_DEFAULT;
	//	desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

	//	hr = device->CreateTexture2D(&desc, nullptr, renderTexture.GetAddressOf());
	//	assert(SUCCEEDED(hr));
	//	hr = device->CreateRenderTargetView(renderTexture.Get(), nullptr, rtv.GetAddressOf());
	//	assert(SUCCEEDED(hr));
	//	hr = device->CreateShaderResourceView(renderTexture.Get(), nullptr, srv.GetAddressOf());
	//	assert(SUCCEEDED(hr));
	//}

	//// 深度テクスチャ生成
	//{
	//	D3D11_TEXTURE2D_DESC desc{};
	//	desc.Width = renderWidth;
	//	desc.Height = renderHeight;
	//	desc.MipLevels = 1;
	//	desc.ArraySize = 1;
	//	desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	//	desc.SampleDesc.Count = 1;
	//	desc.Usage = D3D11_USAGE_DEFAULT;
	//	desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

	//	hr = device->CreateTexture2D(&desc, nullptr, depthTexture.GetAddressOf());
	//	assert(SUCCEEDED(hr));
	//	hr = device->CreateDepthStencilView(depthTexture.Get(), nullptr, dsv.GetAddressOf());
	//	assert(SUCCEEDED(hr));
	//}
}

// 更新
void StudioWindow::Update(float elapsedTime)
{
	// リサイズデバウンス（0.1秒変化がなければ実行）
	if (needsResize)
	{
		resizeTimer += elapsedTime;
		if (resizeTimer >= 0.1f)
		{
			renderWidth = pendingWidth;
			renderHeight = pendingHeight;
			ResizeRenderTarget();
			needsResize = false;
			resizeTimer = 0.0f;
		}
	}

	// カメラ更新
	camera.Update(isHovered, elapsedTime);

	// モデルビュワー確認
	modelViewer.Update(elapsedTime, context);

	// アタッチメント更新
	for (auto& attachment : attachments)
	{
		attachment->Update(elapsedTime, context);
	}
}

// 描画
void StudioWindow::Render()
{
	// ホバー中は毎フレーム、離れたら1/10
	if (!isHovered)
	{
		renderSkipCount++;
		if (renderSkipCount < 10) return;
		renderSkipCount = 0;
	}
	else
	{
		renderSkipCount = 0;
	}

	ID3D11DeviceContext* dc = context.deviceContext;
	if (!dc) return;

	// 描画先を自前RTVに切り替え
	Begin();

	// CbScene更新
	CbScene cbSceneData{};
	cbSceneData.view = camera.GetView();
	cbSceneData.projection = camera.GetProjection();
	const auto& eye = camera.GetEye();
	cbSceneData.cameraPosition = { eye.x, eye.y, eye.z, 0.0f };

	dc->UpdateSubresource(renderSystem.GetCbScene(), 0, nullptr, &cbSceneData, 0, 0);
	dc->VSSetConstantBuffers(0, 1, renderSystem.GetCbSceneAddress());
	dc->PSSetConstantBuffers(0, 1, renderSystem.GetCbSceneAddress());

	// ライトをGPUに転送
	lightManager.Bind(dc);

	// フラスタム更新
	renderSystem.SetFrustum(camera.GetView(), camera.GetProjection());

	// モデルがある時だけRenderSystemを走らせる
	if (!studioRegistry.storage<TransformComponent>().empty())
	{
		// 描画
		renderSystem.Render(
			studioRegistry,
			dc,
			rtv.Get(),
			dsv.Get(),
			context.device,
			renderWidth,
			renderHeight
		);
	}
	// スカイボックス描画
	shared->skyBox.Render(
		dc,
		camera.GetView(),
		camera.GetProjection(),
		camera.GetEye()
	);

	// アタッチメントの3D描画
	for (auto& attachment : attachments)
	{
		attachment->OnRender(
			context.deviceContext,
			camera.GetView(),
			camera.GetProjection(),
			studioRegistry,
			context
		);
	}

	// 描画先をメインに戻す
	End();
}

// ImGui描画
void StudioWindow::DrawWindow(const char* windowName)
{
	//ImGui::Begin(windowName);

	//// ウィンドウがホバーされているか
	//isHovered = ImGui::IsWindowHovered();

	//// 描画領域のサイズを取得してカメラのアスペクト比を更新
	//ImVec2 region = ImGui::GetContentRegionAvail();
	//if (region.x > 0 && region.y > 0)
	//{
	//	camera.UpdateAspect(region.x, region.y);
	//}

	//// 3D描画結果をImGuiに貼り付け
	//ImGui::Image(
	//	reinterpret_cast<ImTextureID>(srv.Get()),
	//	region
	//);

	//// ModelViewerのGUI描画
	//modelViewer.DrawGUI(studioRegistry, context);

	//// アタッチメントのGUI描画
	//for (auto& attachment : attachments)
	//{
	//	attachment->DrawGUI(studioRegistry, context);
	//}

	//ImGui::End();

	bool open = true;
	ImGui::Begin(windowName, &open);
	if (!open) closeRequested = true;

	isHovered = ImGui::IsWindowHovered();

	ImVec2 region = ImGui::GetContentRegionAvail();
	if (region.x > 1 && region.y > 1)
	{
		camera.UpdateAspect(region.x, region.y);

		UINT w = static_cast<UINT>(region.x);
		UINT h = static_cast<UINT>(region.y);

		// サイズが変わっていたらデバウンスタイマーをリセットしてフラグを立てる
		if (w != renderWidth || h != renderHeight)
		{
			pendingWidth = w;
			pendingHeight = h;
			needsResize = true;
			resizeTimer = 0.0f;  // 変化があるたびにリセット
		}
	}

	float dispW = needsResize ? static_cast<float>(pendingWidth) : static_cast<float>(renderWidth);
	float dispH = needsResize ? static_cast<float>(pendingHeight) : static_cast<float>(renderHeight);

	ImGui::Image(
		reinterpret_cast<ImTextureID>(srv.Get()),
		ImVec2(dispW, dispH)
	);

	modelViewer.DrawGUI(studioRegistry, context);

	for (auto& attachment : attachments)
		attachment->DrawGUI(studioRegistry, context);

	ImGui::End();
}

// アタッチメント追加
void StudioWindow::AddAttachment(std::unique_ptr<IStudioAttachment> attachment)
{
	attachments.push_back(std::move(attachment));
}

// アセット読み込み
void StudioWindow::LoadAsset(const std::string& assetPath)
{
	modelViewer.LoadModel(studioRegistry, assetPath);
}

void StudioWindow::Begin()
{
	ID3D11DeviceContext* dc = context.deviceContext;
	if (!dc) return;

	// 現在のビューポートを保存
	UINT numVP = 1;
	dc->RSGetViewports(&numVP, &savedViewport);

	// 自前のビューポートをセット
	D3D11_VIEWPORT vp{};
	vp.Width = static_cast<float>(renderWidth);
	vp.Height = static_cast<float>(renderHeight);
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	dc->RSSetViewports(1, &vp);

	// 描画先を自前RTVに切り替え
	dc->OMSetRenderTargets(1, rtv.GetAddressOf(), dsv.Get());

	// クリア
	float clear[4] = { 0.15f, 0.15f, 0.15f, 1.0f };
	dc->ClearRenderTargetView(rtv.Get(), clear);
	dc->ClearDepthStencilView(dsv.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}
void StudioWindow::End()
{
	ID3D11DeviceContext* dc = context.deviceContext;
	if (!dc) return;

	// ビューポートを元に戻す
	dc->RSSetViewports(1, &savedViewport);

	// 描画先をメインに戻す
	ID3D11RenderTargetView* nullRTV = nullptr;
	dc->OMSetRenderTargets(1, &nullRTV, nullptr);
}
void StudioWindow::ResizeRenderTarget()
{
	ID3D11Device* device = context.device;
	if (!device) return;
	if (renderWidth == 0 || renderHeight == 0) return;

	// 古いリソースを解放
	srv.Reset();
	rtv.Reset();
	renderTexture.Reset();
	dsv.Reset();
	depthTexture.Reset();

	HRESULT hr;

	// カラーテクスチャ再生成
	{
		D3D11_TEXTURE2D_DESC desc{};
		desc.Width = renderWidth;
		desc.Height = renderHeight;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		desc.SampleDesc.Count = 1;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

		hr = device->CreateTexture2D(&desc, nullptr, renderTexture.GetAddressOf());
		assert(SUCCEEDED(hr));
		hr = device->CreateRenderTargetView(renderTexture.Get(), nullptr, rtv.GetAddressOf());
		assert(SUCCEEDED(hr));
		hr = device->CreateShaderResourceView(renderTexture.Get(), nullptr, srv.GetAddressOf());
		assert(SUCCEEDED(hr));
	}

	// 深度テクスチャ再生成
	{
		D3D11_TEXTURE2D_DESC desc{};
		desc.Width = renderWidth;
		desc.Height = renderHeight;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		desc.SampleDesc.Count = 1;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

		hr = device->CreateTexture2D(&desc, nullptr, depthTexture.GetAddressOf());
		assert(SUCCEEDED(hr));
		hr = device->CreateDepthStencilView(depthTexture.Get(), nullptr, dsv.GetAddressOf());
		assert(SUCCEEDED(hr));
	}
}
