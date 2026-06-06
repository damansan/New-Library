#include "framework.h"
#include "tracy/Tracy.hpp"
#include "Game/SceneGame.h"

#include <iomanip>

framework::framework(HWND hwnd) : hwnd(hwnd)
{
	// Graphics初期化
	graphics.Initialize(hwnd);

	// ImGuiManager初期化
	imguiManager.Initialize(hwnd, graphics.GetDevice(), graphics.GetDeviceContext());

	// カメラ初期化
	camera.Initialize(graphics.GetScreenWidth(), graphics.GetScreenHeight());


	// シェーダーパス登録
	ShaderPassLibrary::Register("basic", {
		"", "Data/Shader/Base_VS.cso", "Data/Shader/Basic_PS.cso"
		});
	ShaderPassLibrary::Register("lambert", {
		"", "Data/Shader/Base_VS.cso", "Data/Shader/Lambert_PS.cso"
		});
	ShaderPassLibrary::Register("pbr", {
		"", "Data/Shader/Base_VS.cso", "Data/Shader/PBR_PS.cso"
		});
	ShaderPassLibrary::Register("pbr_skinning", {
	"", "Data/Shader/Skinning_VS.cso", "Data/Shader/PBR_PS.cso"
		});

	// 最初のシーンセット
	sceneManager.ChangeScene(std::make_unique<SceneGame>());

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

	// フリーカメラ更新
	freeCameraController.Update(elapsedTime, camera);

	// コンテキスト生成
	SceneContext context;
	context.device = graphics.GetDevice();
	context.deviceContext = graphics.GetDeviceContext();
	context.resourceManager = &resourceManager;
	context.camera = &camera;
	context.view = camera.GetView();
	context.projection = camera.GetProjection();


	// シーン更新
	sceneManager.Update(elapsedTime, context);
}

void framework::updateImGui()
{
	// ImGuiフレーム開始
	imguiManager.BeginFrame();

	ImGui::Begin("System");
	ImGui::Checkbox("VSync", &vsync);
	ImGui::End();

	// シーンImGui描画
	sceneManager.RenderImGui();
}

void framework::render(float elapsedTime)
{
	ZoneScoped;

	// 画面クリア
	graphics.Clear(0.2f, 0.2f, 0.5f, 1.0f);
	// レンダーターゲット設定
	graphics.SetRenderTargets();


	SceneContext context;
	context.device = graphics.GetDevice();
	context.deviceContext = graphics.GetDeviceContext();
	context.resourceManager = &resourceManager;
	context.renderTargetView = graphics.GetRenderTargetView();
	context.depthStencilView = graphics.GetDepthStencilView();
	context.camera = &camera;
	context.view = camera.GetView();             
	context.projection = camera.GetProjection(); 


	// シーン描画
	sceneManager.Render(context);

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
	case WM_EXITSIZEMOVE:
		timer.start();
		// ウィンドウサイズ変更時にバッファ再生成
		graphics.ResizeBuffers();
		break;
	default:
		return DefWindowProc(hwnd, msg, wparam, lparam);
	}
	return 0;
}