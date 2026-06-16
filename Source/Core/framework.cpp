#include "framework.h"
#include "tracy/Tracy.hpp"
#include "Game/SceneGame.h"
#include "Editor/ComponentRegistryInitializer.h"

#include <iomanip>

framework::framework(HWND hwnd) : hwnd(hwnd)
{
	// Graphics初期化
	graphics.Initialize(hwnd);

	// ImGuiManager初期化
	imguiManager.Initialize(hwnd, graphics.GetDevice(), graphics.GetDeviceContext());

	// カメラ初期化
	camera.Initialize(graphics.GetScreenWidth(), graphics.GetScreenHeight());

	// 最初のシーンセット
	sceneManager.ChangeScene(std::make_unique<SceneGame>());

	// ProjectWindow初期化
	projectWindow.Initialize("Data/Model");
	projectWindow.LoadIcons(graphics.GetDevice());
	ComponentRegistryInitializer::Initialize();

	// Studio共有リソース初期化
	studioShared.Initialize(graphics.GetDevice());

#ifndef _DEBUG
	// リリース時はボーダーレスフルスクリーンで起動
	graphics.ToggleFullscreen();
#endif
}

framework::~framework()
{
	sceneManager.ClearScene();
}

void framework::update(float elapsedTime)
{
	ZoneScoped;

	// 右クリックを押した瞬間Debugモードに切り替え
	static bool prevRButton = false;
	bool currRButton = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
	if (currRButton && !prevRButton)
	{
		editorContext.cameraMode = CameraMode::Debug;
	}
	prevRButton = currRButton;


	// SceneContext 更新
	updateSceneContext();

	// シーン更新
	sceneManager.Update(elapsedTime, sceneContext);

	// Studio更新
	for (auto& win : studioWindows)
	{
		win->Update(elapsedTime);
	}

	// ゲームカメラの結果をcontextに反映
	sceneContext.view = camera.GetView();
	sceneContext.projection = camera.GetProjection();

}

void framework::updateImGui()
{
	// ImGuiフレーム開始
	imguiManager.BeginFrame();

	ImGui::Begin("System");
	ImGui::Checkbox("VSync", &vsync);

	//if (ImGui::Button("Studio Open"))
	//{
	//	openStudioWindow();
	//}
	ImGui::End();

	// シーンImGui描画
	sceneManager.RenderImGui();

#ifdef _DEBUG
	if (auto* scene = sceneManager.GetCurrentScene())
	{
		projectWindow.Draw(scene->GetRegistry(), editorContext);
		hierarchyWindow.Draw(scene->GetRegistry(), editorContext);
		inspectorWindow.Draw(scene->GetRegistry(), editorContext);
	}

	// StudioOpenRequestを処理
	if (editorContext.studioRequest.pending)
	{
		openStudioWindow(editorContext.studioRequest.assetPath);
		editorContext.studioRequest.pending = false;
		editorContext.studioRequest.assetPath = "";
	}
	for (int i = 0; i < (int)studioWindows.size(); i++)
	{
		std::string title = "Studio##" + std::to_string(i);
		studioWindows[i]->DrawWindow(title.c_str());
	}
	// 閉じるリクエストが出たウィンドウを削除
	studioWindows.erase(
		std::remove_if(studioWindows.begin(), studioWindows.end(),
			[](const std::unique_ptr<StudioWindow>& w)
			{
				return w->IsCloseRequested();
			}),
		studioWindows.end()
	);
#endif
}

void framework::render(float elapsedTime)
{
	ZoneScoped;

	// 画面クリア
	graphics.Clear(0.2f, 0.2f, 0.5f, 1.0f);
	// レンダーターゲット設定
	graphics.SetRenderTargets();

	// シーン描画
	sceneManager.Render(sceneContext);

	// Studio描画
	for (auto& win : studioWindows)
	{
		win->Render();
	}

	// メインRTVを再セット
	graphics.SetRenderTargets();

	// ImGui描画
	imguiManager.EndFrame();

	// 画面反映
	graphics.Present(vsync);
	
	FrameMark;
}

// フレーム計算
void framework::calculate_frame_stats()
{
	static int frames = 0;
	static float time_tlapsed = 0.0f;

	frames++;

	if ((timer.time_stamp() - time_tlapsed) >= 1.0f)
	{
		float fps = static_cast<float>(frames);
		float mspf = 1000.0f / fps;

		updateCount = updateCountBuf;
		updateCountBuf = 0;

		std::ostringstream outs;
		outs.precision(6);
		//outs << "FPS : " << fps << " / " << "Frame Time : " << mspf << " (ms)";
		outs << "FPS : " << fps
			<< " / Frame Time : " << mspf << " ms"
			<< " / Update : " << updateCount << " fps";
		SetWindowTextA(hwnd, outs.str().c_str());

		frames = 0;
		time_tlapsed += 1.0f;
	}
}

//アプリケーションループ
int framework::run()
{
	MSG msg{};

	// 固定タイムステップ設定（60fps）
	const double fixedTimestep = 1.0 / 60.0;
	double accumulator = 0.0;

	while (WM_QUIT != msg.message)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			timer.tick();
			calculate_frame_stats();

			// 経過時間を蓄積（最大0.25秒でスパイク防止）
			double frameTime = (double)timer.time_interval();
			if (frameTime > 0.25) frameTime = 0.25;
			accumulator += frameTime;

			// 固定タイムステップでupdateを実行
			while (accumulator >= fixedTimestep)
			{
				update((float)fixedTimestep);
				accumulator -= fixedTimestep;
				updateCountBuf++;
			}

			// ImGuiは毎フレーム1回だけ
			updateImGui();

			// renderは毎フレーム実行
			render((float)timer.time_interval());
		}
	}
	return static_cast<int>(msg.wParam);
}

//メッセージハンドラ
LRESULT framework::handle_message(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	// ImGuiメッセージ処理
	if (imguiManager.HandleMessage(hwnd, msg, wparam, lparam)) { return true; }

	switch (msg)
	{
	case WM_PAINT:
	{
		PAINTSTRUCT ps{};
		BeginPaint(hwnd, &ps);

		EndPaint(hwnd, &ps);
	}
	break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_CREATE:
		break;
	case WM_KEYDOWN:
		if (wparam == VK_ESCAPE)
		{
			PostMessage(hwnd, WM_CLOSE, 0, 0);
		}
		break;
	case WM_ENTERSIZEMOVE:
		timer.stop();
		break;
	case WM_SIZE:
		if (wparam == SIZE_MINIMIZED)
		{
			// 最小化時はタイマー停止のみ（リサイズ不要）
			timer.stop();
		}
		else
		{
			// 最大化/復元/通常リサイズ時
			timer.start();
			graphics.ResizeBuffers();
			updateSceneContext();
		}
		break;
	case WM_EXITSIZEMOVE:
		timer.start();
		// ウィンドウサイズ変更時にバッファ再生成
		graphics.ResizeBuffers();
		updateSceneContext();
		break;
	default:
		return DefWindowProc(hwnd, msg, wparam, lparam);
	}
	return 0;
}

// SceneContext を最新状態に更新する
void framework::updateSceneContext()
{
	// ウィンドウサイズに合わせてカメラのアスペクト比を更新
	camera.SetPerspectiveFov(
		DirectX::XMConvertToRadians(camera.GetViewingAngle()),
		graphics.GetScreenWidth() / graphics.GetScreenHeight(),
		0.1f,
		1000.0f
	);

	sceneContext.device = graphics.GetDevice();
	sceneContext.deviceContext = graphics.GetDeviceContext();
	sceneContext.resourceManager = &resourceManager;
	sceneContext.renderTargetView = graphics.GetRenderTargetView();
	sceneContext.depthStencilView = graphics.GetDepthStencilView();
	sceneContext.camera = &camera;
	sceneContext.view = camera.GetView();
	sceneContext.projection = camera.GetProjection();
	sceneContext.screenWidth = static_cast<UINT>(graphics.GetScreenWidth());
	sceneContext.screenHeight = static_cast<UINT>(graphics.GetScreenHeight());
	sceneContext.cameraMode = editorContext.cameraMode;
	sceneContext.editorContext = &editorContext;
}

// Studioを開く
void framework::openStudioWindow(const std::string& assetPath)
{
	auto win = std::make_unique<StudioWindow>();
	win->Initialize(graphics.GetDevice(), &studioShared);
	win->GetContext().deviceContext = graphics.GetDeviceContext();

	// パスが指定されていたらモデルをロード
	if (!assetPath.empty())
	{
		win->LoadAsset(assetPath);
	}

	studioWindows.push_back(std::move(win));
}