#include "GameApp.h"
#include "tracy/Tracy.hpp"
#include <fstream>
#include <json.hpp>

#include "Engine/Core/Log.h"
#include "Engine/Resource/SceneSerializer.h"
#include "Project/Scene/SceneGame.h"
#include "Project/ECS/ProjectComponents.h"

namespace
{
	// プロジェクト設定ファイル
	const char* kProjectSettingsPath = "Data/ProjectSettings.json";
}

// 初期化
void GameApp::OnInitialize()
{
	// Projectのコンポーネントの登録(Engineのものは基盤が登録済み)
	ProjectComponents::Register();

	sceneManager.ChangeScene(std::make_unique<SceneGame>());

	// 最初に読み込むシーン
	startScenePath = LoadStartScenePath();

#ifndef _DEBUG
	// リリース時はボーダーレスフルスクリーンで起動
	graphics.ToggleFullscreen();
#endif
}

// シーン更新の前
void GameApp::OnPreUpdate(float elapsedTime)
{
	// ウィンドウの縦横比に合わせて、ゲームカメラのプロジェクションを更新する
	RECT rc{};
	GetClientRect(hwnd, &rc);
	float width = static_cast<float>(rc.right - rc.left);
	float height = static_cast<float>(rc.bottom - rc.top);
	if (width > 0.0f && height > 0.0f)
	{
		gameView.SetAspect(width / height);
	}
}

// シーン更新の後
void GameApp::OnPostUpdate(float elapsedTime)
{
	// SceneGameは最初のシーン更新で初期化されるので、その後に1回だけ読み込む
	if (startSceneLoaded) return;

	Scene* scene = sceneManager.GetCurrentScene();
	if (!scene) return;

	startSceneLoaded = true;

	if (startScenePath.empty()) return;

	// エンティティの名前はエディタ専用なので、受け取らない
	if (!SceneSerializer::LoadFromFile(scene->GetRegistry(), startScenePath))
	{
		Log(u8"最初のシーンの読み込みに失敗しました: %s", startScenePath.c_str());
	}
}

// GameViewに描いたゲームを、画面全体に貼る
void GameApp::OnPresent()
{
	ZoneScopedN("PresentTexture");
	graphics.PresentTexture(gameView.GetSRV());
}

// プロジェクト設定から、最初に読み込むシーンのパスを取得する
std::string GameApp::LoadStartScenePath()
{
	std::ifstream file(kProjectSettingsPath);
	if (!file)
	{
		Log(u8"プロジェクト設定が見つかりません: %s", kProjectSettingsPath);
		return "";
	}

	nlohmann::json json = nlohmann::json::parse(file, nullptr, false);
	if (json.is_discarded() || !json.is_object())
	{
		Log(u8"プロジェクト設定の形式が正しくありません: %s", kProjectSettingsPath);
		return "";
	}

	std::string path = json.value("startScene", "");
	if (path.empty())
	{
		Log(u8"プロジェクト設定に startScene がありません");
	}
	return path;
}