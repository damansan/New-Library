#include "StudioWindow.h"
#include <imgui.h>
#include <imgui_internal.h>
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

	stadoiModelViewer.Initialize(device, &shared->resourceManager);

	ResizeRenderTarget();
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

	// モデルビュワー更新
	stadoiModelViewer.Update(elapsedTime, context);

	if (context.requestPoseUpdate)
	{
		// スクラブ操作時は時間を進めずポーズだけ更新
		animationSystem.EvaluatePose(studioRegistry);
		context.requestPoseUpdate = false;
	}
	else
	{
		animationSystem.Update(studioRegistry, elapsedTime);
	}

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
	bool open = true;
	ImGui::Begin(windowName, &open, ImGuiWindowFlags_MenuBar);
	if (!open) closeRequested = true;

	// Studioウィンドウ全体でホバー判定
	isHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows | ImGuiHoveredFlags_DockHierarchy);

	// メニューバー
	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("Window"))
		{
			ImGui::MenuItem("Animator", nullptr, &context.showAnimator);
			ImGui::MenuItem("Timeline", nullptr, &context.showTimeline);
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}

	// このStudio専用のDockSpace
	std::string dockSpaceName = "StudioDockSpace##" + std::to_string(windowId);
	ImGuiID dockSpaceId = ImGui::GetID(dockSpaceName.c_str());
	ImGui::DockSpace(dockSpaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

	// 初回フレームだけデフォルトレイアウトを組む
	if (firstDockSetup)
	{
		firstDockSetup = false;

		ImGui::DockBuilderRemoveNode(dockSpaceId);
		ImGui::DockBuilderAddNode(dockSpaceId, ImGuiDockNodeFlags_None);
		ImGui::DockBuilderSetNodeSize(dockSpaceId, ImVec2(1280, 720));

		ImGuiID dockMain = dockSpaceId;
		ImGuiID dockLeft = ImGui::DockBuilderSplitNode(
			dockMain, ImGuiDir_Left, 0.2f, nullptr, &dockMain);
		ImGuiID dockBottom = ImGui::DockBuilderSplitNode(
			dockMain, ImGuiDir_Down, 0.2f, nullptr, &dockMain);

		std::string viewportName = "Viewport##" + std::to_string(windowId);
		std::string animatorName = "Animator##" + std::to_string(windowId);
		std::string timelineName = "Timeline##" + std::to_string(windowId);

		ImGui::DockBuilderDockWindow(viewportName.c_str(), dockMain);
		ImGui::DockBuilderDockWindow(animatorName.c_str(), dockLeft);
		ImGui::DockBuilderDockWindow(timelineName.c_str(), dockBottom);

		ImGui::DockBuilderFinish(dockSpaceId);
	}

	// Viewportパネル
	std::string viewportName = "Viewport##" + std::to_string(windowId);
	ImGui::Begin(viewportName.c_str());

	ImVec2 region = ImGui::GetContentRegionAvail();
	if (region.x > 1 && region.y > 1)
	{
		camera.UpdateAspect(region.x, region.y);

		UINT w = static_cast<UINT>(region.x);
		UINT h = static_cast<UINT>(region.y);

		if (isFirstFrame)
		{
			isFirstFrame = false;
		}
		else if (w != renderWidth || h != renderHeight)
		{
			pendingWidth = w;
			pendingHeight = h;
			needsResize = true;
			resizeTimer = 0.0f;
		}
	}

	ImGui::Image(reinterpret_cast<ImTextureID>(srv.Get()), region);

	ImGui::End(); // Viewport

	// Animatorパネル
	if (context.showAnimator)
	{
		studioAnimator.DrawGUI(studioRegistry, context);
	}

	// Timelineパネル
	if (context.showTimeline)
	{
		studioTimeline.DrawGUI(studioRegistry, context);
	}

	stadoiModelViewer.DrawGUI(studioRegistry, context);

	for (auto& attachment : attachments)
		attachment->DrawGUI(studioRegistry, context);

	ImGui::End(); // Studio
}

// アタッチメント追加
void StudioWindow::AddAttachment(std::unique_ptr<IStudioAttachment> attachment)
{
	attachments.push_back(std::move(attachment));
}

// アセット読み込み
void StudioWindow::LoadAsset(const std::string& assetPath)
{
	stadoiModelViewer.LoadModel(studioRegistry, assetPath);
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
