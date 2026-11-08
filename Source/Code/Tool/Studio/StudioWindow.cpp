#include "StudioWindow.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <cassert>
#include <cmath>

namespace
{
	// 最大化したときの広げる先を求める
	void GetMaximizeArea(ImVec2 windowPos, ImVec2 windowSize, ImVec2& outPos, ImVec2& outSize)
	{
		// アプリの中にいる
		ImGuiViewport* mainViewport = ImGui::GetMainViewport();
		if (ImGui::GetWindowViewport() == mainViewport)
		{
			outPos = mainViewport->WorkPos;
			outSize = mainViewport->WorkSize;
			return;
		}

		// アプリの外：ウィンドウの中心があるモニターを探す
		ImVec2 center(windowPos.x + windowSize.x * 0.5f, windowPos.y + windowSize.y * 0.5f);
		const ImGuiPlatformIO& platformIO = ImGui::GetPlatformIO();
		for (const ImGuiPlatformMonitor& monitor : platformIO.Monitors)
		{
			bool inside =
				center.x >= monitor.MainPos.x && center.x < monitor.MainPos.x + monitor.MainSize.x &&
				center.y >= monitor.MainPos.y && center.y < monitor.MainPos.y + monitor.MainSize.y;
			if (inside)
			{
				outPos = monitor.WorkPos;
				outSize = monitor.WorkSize;
				return;
			}
		}

		// 見つからなければ最初のモニター
		if (!platformIO.Monitors.empty())
		{
			outPos = platformIO.Monitors[0].WorkPos;
			outSize = platformIO.Monitors[0].WorkSize;
			return;
		}

		// モニターの情報が無い場合は、アプリのウィンドウ
		outPos = mainViewport->WorkPos;
		outSize = mainViewport->WorkSize;
	}

	// 2つの値がほぼ同じか
	bool NearlyEqual(ImVec2 a, ImVec2 b)
	{
		return std::fabs(a.x - b.x) < 1.0f && std::fabs(a.y - b.y) < 1.0f;
	}
}

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
	stateGraph.Initialize();

	ResizeRenderTarget();
}

// 更新
void StudioWindow::Update(float elapsedTime)
{
	// カメラ更新
	camera.Update(viewportHovered, elapsedTime);

	// モデルビュワー更新
	stadoiModelViewer.Update(elapsedTime, context);

	// アニメーション速度更新
	animationSpeedSystem.Update(studioRegistry);

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

	// スカイボックス描画
	shared->skyBox.Render(
		dc,
		camera.GetView(),
		camera.GetProjection(),
		camera.GetEye()
	);

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

	// 描画先をメインに戻す
	End();
}

// ImGui描画
void StudioWindow::DrawWindow(const char* windowName)
{
	// 最大化・元に戻すの予約があれば、Beginの前に位置と大きさを伝える
	if (applyWindowRect)
	{
		ImGui::SetNextWindowPos(applyPos, ImGuiCond_Always);
		ImGui::SetNextWindowSize(applySize, ImGuiCond_Always);
		applyWindowRect = false;
	}


	bool open = true;
	ImGui::Begin(windowName, &open, ImGuiWindowFlags_MenuBar);
	if (!open) closeRequested = true;

	// Studioがいるビューポート
	context.studioViewportId = ImGui::GetWindowViewport()->ID;

	// 最大化中に、ドラッグで動かした・大きさを変えた・ドッキングしたら、最大化を外す
	if (maximized)
	{
		bool changed = !NearlyEqual(ImGui::GetWindowPos(), applyPos) || !NearlyEqual(ImGui::GetWindowSize(), applySize);
		if (changed || ImGui::IsWindowDocked()) maximized = false;
	}

	// 右端：最大化ボタン
	DrawMaximizeButton();

	// Studioウィンドウ全体でホバー判定
	isHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows | ImGuiHoveredFlags_DockHierarchy);

	// メニューバー
	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("Window"))
		{
			ImGui::MenuItem("Animator", nullptr, &context.showAnimator);
			ImGui::MenuItem("Timeline", nullptr, &context.showTimeline);
			ImGui::MenuItem("Sequencer", nullptr, &context.showSequencer);
			ImGui::MenuItem("State Graph", nullptr, &context.showStateGraph);
			ImGui::MenuItem("AnimationSpeed", nullptr, &context.showAnimationSpeed);
			ImGui::MenuItem("Animation Edit", nullptr, &context.showAnimationEdit);

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
		ImGuiID dockRight = ImGui::DockBuilderSplitNode(
			dockMain, ImGuiDir_Right, 0.7f, nullptr, &dockMain);
		ImGuiID dockBottom = ImGui::DockBuilderSplitNode(
			dockMain, ImGuiDir_Down, 0.2f, nullptr, &dockMain);
		ImGuiID dockSequencer = ImGui::DockBuilderSplitNode(
			dockBottom, ImGuiDir_Down, 0.5f, nullptr, &dockBottom);

		std::string viewportName = "Viewport##" + std::to_string(windowId);
		std::string animatorName = "Animator##" + std::to_string(windowId);
		std::string timelineName = "Timeline##" + std::to_string(windowId);
		std::string sequencerName = "Sequencer##" + std::to_string(windowId);
		std::string stateGraphName = "StateGraph##" + std::to_string(windowId);
		std::string animationSpeedName = "Animation Speed##" + std::to_string(windowId);

		ImGui::DockBuilderDockWindow(viewportName.c_str(), dockMain);
		ImGui::DockBuilderDockWindow(animatorName.c_str(), dockLeft);
		ImGui::DockBuilderDockWindow(timelineName.c_str(), dockBottom);
		ImGui::DockBuilderDockWindow(sequencerName.c_str(), dockSequencer);
		ImGui::DockBuilderDockWindow(stateGraphName.c_str(), dockRight);
		ImGui::DockBuilderDockWindow(animationSpeedName.c_str(), dockSequencer);

		ImGui::DockBuilderFinish(dockSpaceId);
	}

	// Viewportパネル
	std::string viewportName = "Viewport##" + std::to_string(windowId);
	ImGui::Begin(viewportName.c_str());

	// 画質確認
	// ImGui::Text("Render Resolution: %ux%u", renderWidth, renderHeight);

	ImVec2 region = ImGui::GetContentRegionAvail();
	if (region.x > 1 && region.y > 1)
	{
		camera.UpdateAspect(region.x, region.y);
	}

	ImGui::Image(reinterpret_cast<ImTextureID>(srv.Get()), region);

	// モデルのドロップを受け取る
	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MODEL_PATH"))
		{
			std::string droppedModelPath(static_cast<const char*>(payload->Data));
			LoadAsset(droppedModelPath);
		}
		ImGui::EndDragDropTarget();
	}

	// カメラ操作の開始判定はViewport単体のホバーだけを見る
	viewportHovered = ImGui::IsWindowHovered();

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

	// Sequencerパネル
	if (context.showSequencer)
	{
		studioSequencer.DrawGUI(studioRegistry, context);
	}

	// State Graphパネル
	if (context.showStateGraph)
	{
		stateGraph.DrawGUI(studioRegistry, context);
	}

	// AnimationSpeedパネル
	if (context.showAnimationSpeed)
	{
		studioAnimationSpeed.DrawGUI(studioRegistry, context);
	}
	
	// Animation Editパネル
	if (context.showAnimationEdit)
	{
		studioAnimationEdit.DrawGUI(studioRegistry, context);
	}
	else
	{
		studioAnimationEdit.OnHidden(studioRegistry, context);
	}

	stadoiModelViewer.DrawGUI(studioRegistry, context);

	ImGui::End(); // Studio
}

// アセット読み込み
void StudioWindow::LoadAsset(const std::string& assetPath)
{
	stadoiModelViewer.LoadModel(studioRegistry, assetPath);
	context.currentModelPath = assetPath;

	// モデルが変わったことを伝える
	stateGraph.OnModelChanged(context);
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

// 最大化ボタン
void StudioWindow::DrawMaximizeButton()
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();

	// ドッキング中はタイトルバーがタブになるので出さない
	if (window->DockIsActive || (window->Flags & ImGuiWindowFlags_NoTitleBar)) return;

	ImGuiStyle& style = ImGui::GetStyle();
	ImRect titleBar = window->TitleBarRect();

	// ×ボタンと同じ大きさで、×の左に並べる(ImGuiが×を置く位置の計算に合わせる)
	float buttonSize = ImGui::GetFontSize();
	float closeX = titleBar.Max.x - window->WindowBorderSize - style.FramePadding.x - buttonSize;
	ImVec2 pos(closeX - style.ItemInnerSpacing.x - buttonSize, titleBar.Min.y + style.FramePadding.y);
	ImRect bb(pos, ImVec2(pos.x + buttonSize, pos.y + buttonSize));

	// タイトルバーは中身の描画範囲の外なので、ボタンを作る間だけ範囲を広げる
	ImGui::PushClipRect(titleBar.Min, titleBar.Max, false);

	// ボタンの判定(×ボタンと同じ作り方)
	ImGuiID id = window->GetID("#MAXIMIZE");
	ImGui::ItemAdd(bb, id);
	bool hovered = false;
	bool held = false;
	bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);

	// マウスが乗ったら、×ボタンと同じく丸い背景を出す
	ImDrawList* drawList = window->DrawList;
	ImVec2 center = bb.GetCenter();
	if (hovered)
	{
		ImU32 bg = ImGui::GetColorU32(held ? ImGuiCol_ButtonActive : ImGuiCol_ButtonHovered);
		drawList->AddCircleFilled(center, ImMax(2.0f, buttonSize * 0.5f + 1.0f), bg);
	}

	// 印：通常は四角1つ(最大化)、最大化中は四角2つを重ねる(元に戻す)
	ImU32 color = ImGui::GetColorU32(ImGuiCol_Text);
	float half = buttonSize * 0.25f;
	if (!maximized)
	{
		drawList->AddRect(ImVec2(center.x - half, center.y - half), ImVec2(center.x + half, center.y + half), color);
	}
	else
	{
		float offset = half * 0.4f;
		drawList->AddRect(ImVec2(center.x - half + offset, center.y - half - offset), ImVec2(center.x + half + offset, center.y + half - offset), color);
		drawList->AddRect(ImVec2(center.x - half - offset, center.y - half + offset), ImVec2(center.x + half - offset, center.y + half + offset), color);
	}

	ImGui::PopClipRect();

	// 説明
	if (hovered)
	{
		ImGui::SetTooltip("%s", maximized ? "Restore" : "Maximize");
	}

	if (!pressed) return;

	if (!maximized)
	{
		// 今の位置と大きさを覚えて、広げる先を予約する
		restorePos = ImGui::GetWindowPos();
		restoreSize = ImGui::GetWindowSize();
		GetMaximizeArea(restorePos, restoreSize, applyPos, applySize);
		maximized = true;
	}
	else
	{
		// 覚えておいた位置と大きさに戻す
		applyPos = restorePos;
		applySize = restoreSize;
		maximized = false;
	}

	// 位置と大きさは、次のフレームのBeginの前に反映する
	applyWindowRect = true;
}
