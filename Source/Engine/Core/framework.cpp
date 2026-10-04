#include "framework.h"
#include "tracy/Tracy.hpp"
#include "Project/Scene/SceneGame.h"
#include "Tool/Editor/ComponentRegistryInitializer.h"
#include "Tool/Editor/FileDialog.h"

#include "Engine/Resource/SceneSerializer.h"

#include <filesystem>
#include <iomanip>

framework::framework(HWND hwnd) : hwnd(hwnd)
{
	// Graphics初期化
	graphics.Initialize(hwnd);
	
	// PhysX初期化
	physX.Initialize();

	// ImGuiManager初期化
	imguiManager.Initialize(hwnd, graphics.GetDevice(), graphics.GetDeviceContext());

	// Gameウィンドウ初期化
	gameWindow.Initialize(graphics.GetDevice());
	
	// 最初のシーンセット
	sceneManager.ChangeScene(std::make_unique<SceneGame>());

	// ProjectWindow初期化
	projectWindow.Initialize("Data");
	projectWindow.LoadIcons(graphics.GetDevice());
	// PrefabWindow初期化
	prefabWindow.Initialize("Data/Prefab");


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
	
	// PhysX終了処理
	physX.Cleanup();

	// Graphics終了処理
	graphics.Cleanup();
}

void framework::update(float elapsedTime)
{
	ZoneScoped;

#ifdef _DEBUG
	// F1でReleaseと同じ見た目（ImGui無し）をプレビュー切り替え
	static bool prevF1 = false;
	bool currF1 = (GetAsyncKeyState(VK_F1) & 0x8000) != 0;
	if (currF1 && !prevF1)
	{
		previewRelease = !previewRelease;
	}
	prevF1 = currF1;
#endif

	// 右クリックを押した瞬間、Gameへの入力が有効であればDebugモードに切り替え
	static bool prevRButton = false;
	bool currRButton = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
	if (currRButton && !prevRButton && IsGameInputActive())
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
	sceneContext.view = gameWindow.GetCamera().GetView();
	sceneContext.projection = gameWindow.GetCamera().GetProjection();
}

void framework::updateImGui()
{
	ZoneScoped;

	// Editor無効時はImGui自体を一切使わない
	if (!IsEditorActive()) return;

	// ImGuiフレーム開始
	imguiManager.BeginFrame();

	// シーンImGui描画
	sceneManager.RenderImGui();

	// Gameウィンドウ（ドッキング/タブ化対象）メニュー内容
	gameWindow.DrawWindow("Game", [this]() { drawMainMenu(); });
#ifdef _DEBUG
	if (auto* scene = sceneManager.GetCurrentScene())
	{
		projectWindow.Draw(scene->GetRegistry(), editorContext);
		hierarchyWindow.Draw(scene->GetRegistry(), editorContext);
		inspectorWindow.Draw(scene->GetRegistry(), editorContext);
		prefabWindow.Draw(scene->GetRegistry(), editorContext);
	}
	// シーンが無くても、ログは見られるようにする
	logWindow.Draw();

	// StudioOpenRequestを処理
	if (editorContext.studioRequest.pending)
	{
		openStudioWindow(editorContext.studioRequest.assetPath, editorContext.studioRequest.graphPath);
		editorContext.studioRequest.pending = false;
		editorContext.studioRequest.assetPath = "";
		editorContext.studioRequest.graphPath = "";
	}
	for (auto& win : studioWindows)
	{
		std::string title = "Studio##" + std::to_string(win->GetWindowId());
		win->DrawWindow(title.c_str());
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
	// 全てのウィンドウの描画後にシーン保存・読み込みを実行する
	processSceneRequest();

	// Openの確認ポップアップ
	if (ImGui::BeginPopupModal("Open Scene##Confirm", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("Current scene will be discarded.\nAre you sure you want to continue?");
		ImGui::Spacing();

		if (ImGui::Button("Open", ImVec2(120, 0)))
		{
			Scene* scene = sceneManager.GetCurrentScene();
			if (scene)
			{
				// 破棄する古いEntityへの参照が残らないよう、読み込みの前に、選択・名前マップをリセットする
				editorContext.Deselect();
				editorContext.entityNames.clear();

				auto setName = [this](entt::entity e, const std::string& name) { editorContext.SetName(e, name); };

				if (SceneSerializer::LoadFromFile(scene->GetRegistry(), pendingOpenPath, setName))
				{
					editorContext.currentScenePath = pendingOpenPath;
				}
			}
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0)))
		{
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

void framework::render(float elapsedTime)
{
	ZoneScoped;

	// 画面クリア
	graphics.Clear(0.2f, 0.2f, 0.5f, 1.0f);
	graphics.SetRenderTargets();

	// Game描画先をGameWindow用テクスチャに切り替えて描画
	gameWindow.Begin(graphics.GetDeviceContext());
	sceneManager.Render(sceneContext);
	{
		ZoneScopedN("GameWindow::End");
		gameWindow.End(graphics.GetDeviceContext());
	}

	if (IsEditorActive())
	{
		// Studio描画
		{
			ZoneScopedN("Studio::Render");
			for (auto& win : studioWindows)
			{
				win->Render();
			}
		}

		// メインRTVを再セット
		graphics.SetRenderTargets();

		// ImGui描画（GameウィンドウもここでImGui::Image経由で表示される）
		{
			ZoneScopedN("ImGui::EndFrame");
			imguiManager.EndFrame();
		}
	}
	else
	{
		// Editor無効時：Gameのテクスチャをフルスクリーンでバックバッファへ
		graphics.SetRenderTargets();
		{
			ZoneScopedN("PresentTexture");
			graphics.PresentTexture(gameWindow.GetSRV());
		}
	}

	// 画面反映
	{
		ZoneScopedN("Present");
		graphics.Present(vsync);
	}

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

	// 固定タイムステップ設定（144fps）
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
	sceneContext.device = graphics.GetDevice();
	sceneContext.deviceContext = graphics.GetDeviceContext();
	sceneContext.resourceManager = &resourceManager;
	sceneContext.physX = &physX;

	// GameWindowが持つ専用テクスチャを描画先として渡す
	sceneContext.renderTargetView = gameWindow.GetRTV();
	sceneContext.depthStencilView = gameWindow.GetDSV();
	sceneContext.camera = &gameWindow.GetCamera();
	sceneContext.view = gameWindow.GetCamera().GetView();
	sceneContext.projection = gameWindow.GetCamera().GetProjection();
	sceneContext.screenWidth = gameWindow.GetRenderWidth();
	sceneContext.screenHeight = gameWindow.GetRenderHeight();
	sceneContext.cameraMode = editorContext.cameraMode;
	sceneContext.editorContext = &editorContext;
	sceneContext.cameraMode = editorContext.cameraMode;
	sceneContext.editorContext = &editorContext;
	sceneContext.gameInputActive = IsGameInputActive();
}

// Studioを開く
void framework::openStudioWindow(const std::string& assetPath, const std::string& graphPath)
{
	auto win = std::make_unique<StudioWindow>();
	win->Initialize(graphics.GetDevice(), &studioShared);
	win->GetContext().deviceContext = graphics.GetDeviceContext();
	win->SetWindowId(nextStudioWindowId++);

	// パスが指定されていたらモデルをロード
	if (!assetPath.empty())
	{
		win->LoadAsset(assetPath);
	}

	// グラフのパスが指定されていたら紐付け状態にする
	if (!graphPath.empty())
	{
		win->GetContext().attachedGraphPath = graphPath;
	}

	studioWindows.push_back(std::move(win));
}

// メニューバーの中身
void framework::drawMainMenu()
{
	if (ImGui::BeginMenu("File"))
	{
		if (ImGui::MenuItem("Save"))
		{
			editorContext.sceneRequest = SceneAction::Save;
		}
		if (ImGui::MenuItem("Save As..."))
		{
			editorContext.sceneRequest = SceneAction::SaveAs;
		}
		if (ImGui::MenuItem("Open Scene..."))
		{
			editorContext.sceneRequest = SceneAction::Open;
		}

		ImGui::Separator();

		if (ImGui::BeginMenu("System"))
		{
			ImGui::MenuItem("VSync", nullptr, &vsync);
			ImGui::EndMenu();
		}

		ImGui::EndMenu();
	}

	// 今開いているシーン（Saveの上書き先）
	std::string sceneName = editorContext.currentScenePath.empty()
		? "(unsaved)"
		: std::filesystem::path(editorContext.currentScenePath).filename().string();
	ImGui::TextDisabled("Scene: %s", sceneName.c_str());
}

// シーンの保存・読み込みリクエストを実行する
void framework::processSceneRequest()
{
	SceneAction action = editorContext.sceneRequest;
	if (action == SceneAction::None) return;
	editorContext.sceneRequest = SceneAction::None;

	Scene* scene = sceneManager.GetCurrentScene();
	if (!scene) return;

	if (action == SceneAction::Open)
	{
		timer.stop();
		std::string path;
		bool selected = FileDialog::OpenScene(hwnd, path);
		timer.start();

		if (selected)
		{
			// 既存のEntityを破棄するため、確認ポップアップを次のフレームで開く
			pendingOpenPath = path;
			ImGui::OpenPopup("Open Scene##Confirm");
		}
		return;
	}

	std::string path = editorContext.currentScenePath;

	// Save As、またはまだ保存先が無いSaveは、ダイアログで保存先を選ぶ
	if (action == SceneAction::SaveAs || path.empty())
	{
		timer.stop();
		bool selected = FileDialog::SaveScene(hwnd, path);
		timer.start();

		if (!selected) return; // キャンセル
}

	auto getName = [this](entt::entity e) { return editorContext.GetName(e); };
	if (SceneSerializer::SaveToFile(scene->GetRegistry(), path, getName))
	{
		editorContext.currentScenePath = path;
	}
}

bool framework::IsEditorActive() const
{
#ifdef _DEBUG
	return !previewRelease;
#else
	return false;
#endif
}

bool framework::IsGameInputActive() const
{
	return IsEditorActive() ? gameWindow.IsViewportHovered() : true;
}
