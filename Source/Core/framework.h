#pragma once

#include <windows.h>
#include <tchar.h>
#include <sstream>

#include "Utility/misc.h"
#include "Utility/high_resolution_timer.h"

#include "Core/Graphics.h"
#include "Core/ImGuiManager.h"

// ウィンドウサイズ
#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

class framework
{
public:
	framework(HWND hwnd);
	~framework();


	framework(const framework&) = delete;                // コピーコンストラクタ禁止
	framework& operator=(const framework&) = delete;     // コピー代入禁止
	framework(framework&&) noexcept = delete;            // ムーブコンストラクタ禁止
	framework& operator=(framework&&) noexcept = delete; // ムーブ代入禁止


	void update(float elapsed_time);
	void render(float elapsed_time);

	// フレーム計算
	void calculate_frame_stats();

	//アプリケーションループ
	int run();

	//メッセージハンドラ
	LRESULT handle_message(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

private:
	HWND hwnd;
	high_resolution_timer timer;

	// Graphicsクラス
	Graphics graphics;
	// ImGuiManagerクラス
	ImGuiManager imguiManager;
};

