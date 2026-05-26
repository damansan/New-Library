#include "framework.h"

#include "Game/SceneGame.h"

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
	ShaderPassLibrary::Register("skinning_basic", {
	"", "Data/Shader/Skinning_VS.cso", "Data/Shader/Basic_PS.cso"
		});
	ShaderPassLibrary::Register("skinning_lambert", {
		"", "Data/Shader/Skinning_VS.cso", "Data/Shader/Lambert_PS.cso"
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
}

void framework::update(float elapsedTime)
{
	// ImGuiフレーム開始
	imguiManager.BeginFrame();

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
	// シーンImGui描画
	sceneManager.RenderImGui();
}

void framework::render(float elapsedTime)
{
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
	graphics.Present();
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
		std::ostringstream outs;
		outs.precision(6);
		outs << "FPS : " << fps << " / " << "Frame Time : " << mspf << " (ms)";
		SetWindowTextA(hwnd, outs.str().c_str());

		frames = 0;
		time_tlapsed += 1.0f;
	}
}

//アプリケーションループ
int framework::run()
{
	MSG msg{};

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
			update(timer.time_interval());
			render(timer.time_interval());
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