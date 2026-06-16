#pragma once

#include <windows.h>
#include <tchar.h>
#include <sstream>

#include "Utility/misc.h"
#include "Utility/high_resolution_timer.h"

#include "Core/Graphics.h"
#include "Core/ImGuiManager.h"
#include "Core/SceneManager.h"
#include "Core/Camera.h"

#include "Resource/ResourceManager.h"

#include "Editor/EditorContext.h"
#include "Editor/ProjectWindow.h"
#include "Editor/HierarchyWindow.h"
#include "Editor/InspectorWindow.h"

#include "Tool/Studio/StudioSharedContext.h"
#include "Tool/Studio/StudioWindow.h"
#include <vector>

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


	void update(float elapsedTime);
	void updateImGui();
	void render(float elapsedTime);

	// フレーム計算
	void calculate_frame_stats();

	//アプリケーションループ
	int run();

	//メッセージハンドラ
	LRESULT handle_message(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

private:
	HWND hwnd;
	high_resolution_timer timer;

	// SceneContext
	SceneContext sceneContext;
	// SceneContext を最新状態に更新する
	void updateSceneContext();

	// Graphicsクラス
	Graphics graphics;
	// ImGuiManagerクラス
	ImGuiManager imguiManager;

	// SceneManagerクラス
	SceneManager sceneManager;
	// ResourceManagerクラス
	ResourceManager resourceManager;
	// Cameraクラス
	Camera camera;

	// Editor
	EditorContext  editorContext;
	ProjectWindow  projectWindow;
	HierarchyWindow hierarchyWindow;
	InspectorWindow inspectorWindow;

	// Studio
	StudioSharedContext        studioShared;
	std::vector<std::unique_ptr<StudioWindow>> studioWindows;
	void openStudioWindow(const std::string& assetPath = "");
	int nextStudioWindowId = 0;
private:
	// VSync ON OFF
	bool vsync = true;

	// Update計測
	int updateCount = 0;
	int updateCountBuf = 0;
};