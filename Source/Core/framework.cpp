#include "framework.h"

framework::framework(HWND hwnd) : hwnd(hwnd)
{
	// Graphics初期化
	graphics.Initialize(hwnd);

	// ImGuiManager初期化
	imguiManager.Initialize(
		hwnd,
		graphics.GetDevice(),
		graphics.GetDeviceContext()
	);

#ifndef _DEBUG
	// リリース時はボーダーレスフルスクリーンで起動
	graphics.ToggleFullscreen();
#endif

}

framework::~framework()
{
}

void framework::update(float elapsed_time)
{
	// ImGuiフレーム開始
	imguiManager.BeginFrame();

	ImGui::Begin("Test");

	ImGui::End();
}
void framework::render(float elapsed_time)
{
	// 画面クリア
	graphics.Clear(0.2f, 0.2f, 0.2f, 1.0f);
	// レンダーターゲット設定
	graphics.SetRenderTargets();

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