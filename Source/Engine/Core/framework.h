#pragma once

#include <windows.h>
#include <tchar.h>
#include <sstream>

#include "Engine/Utility/misc.h"
#include "Engine/Utility/high_resolution_timer.h"

#include "Engine/Core/Graphics.h"
#include "Engine/Core/ImGuiManager.h"
#include "Engine/Core/SceneManager.h"
#include "Engine/Core/GameWindow.h"
#include "Engine/Core/PhysX.h"

#include "Engine/Resource/ResourceManager.h"

#include "Tool/Editor/EditorContext.h"
#include "Tool/Editor/ProjectWindow.h"
#include "Tool/Editor/HierarchyWindow.h"
#include "Tool/Editor/InspectorWindow.h"
#include "Tool/Editor/PrefabWindow.h"
#include "Tool/Editor/LogWindow.h"

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
	// Gameウィンドウ（描画先テクスチャ＋カメラ）
	GameWindow gameWindow;
	// PhysXクラス
	PhysX physX;


	// Editor
	EditorContext   editorContext;
	ProjectWindow   projectWindow;
	HierarchyWindow hierarchyWindow;
	InspectorWindow inspectorWindow;
	PrefabWindow    prefabWindow;
	LogWindow       logWindow;

	// Studio
	StudioSharedContext        studioShared;
	std::vector<std::unique_ptr<StudioWindow>> studioWindows;
	void openStudioWindow(const std::string& assetPath = "", const std::string& graphPath = "");
	int nextStudioWindowId = 0;

	// メニューバーの中身
	void drawMainMenu();
	// メニューが立てた、シーンの保存・読み込みリクエストを実行する
	void processSceneRequest();

	// Openの確認ポップアップ
	std::string pendingOpenPath;

private:
	// VSync ON OFF
	bool vsync = true;

	// Update計測
	int updateCount = 0;
	int updateCountBuf = 0;


#ifdef _DEBUG
	bool previewRelease = false;
#endif
	bool IsEditorActive() const;
	bool IsGameInputActive() const;

};