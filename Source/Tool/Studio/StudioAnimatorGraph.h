#pragma once
#include "IStudioAttachment.h"
#include "Tool/AnimatorGraph.h"
#include "Tool/NodeEditorCanvas.h"
#include <vector>
#include <string>
#include <imgui.h>


class StudioAnimatorGraph : public IStudioAttachment
{
public:
	StudioAnimatorGraph() = default;
	~StudioAnimatorGraph() = default;

	void Initialize();
	void SetWindowId(int id) { windowId = id; }

	// ImGui描画
	void DrawGUI(
		entt::registry& registry,
		StudioContext& context) override;

private:
	// 現在読み込まれているモデルのアニメーション一覧を取得
	void RefreshAvailableAnimations(entt::registry& registry);

	// 保存
	void Save(StudioContext& context);
	// 読み込み
	void Load(StudioContext& context);
	// D&D受け取り口
	void TryAttachGraph(const std::string& path, StudioContext& context);

	NodeEditorCanvas canvas;
	AnimatorGraph graph; // 編集対象データ
	int windowId = 0;

	// アニメーション一覧
	std::vector<std::string> availableAnimations;
	// 右クリック位置
	ImVec2 pendingCreatePos{};

	// 読み込み済フラグ
	bool loaded = false;
	// 位置反映フラグ
	bool needsApplyPositions = false;

	// D&D紐付けに失敗時のエラーメッセージ
	std::string attachError;
};