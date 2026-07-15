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
	// 上側ツールバー
	void DrawToolbar(StudioContext& context);


	// .animgraphのD&D受け口
	void DrawDropTarget(StudioContext& context);
	void TryAttachGraph(const std::string& path, StudioContext& context);


	// State描画
	void DrawNodes();
	void PushStateStyle(const AnimState& state);
	void PopStateStyle(const AnimState& state);
	void DrawHexagonBackground(ed::NodeId nodeId, const AnimState& state);

	// Link描画
	void DrawLinks();
	// 辺からリンクを伸ばす
	ImVec2 GetEdgePoint(const ImVec2& rectMin, const ImVec2& rectMax, const ImVec2& target);

	// 右クリックメニュー
	void DrawCreateNodeMenu();
	// 右クリックリンク作成メニュー
	void DrawNodeContextMenu();
	// リンク作成モード中のクリック判定
	void UpdateLinkCreation();

	// 右側メニュー
	void DrawInspector(AnimState* selected);
	// 選択中State取得
	AnimState* GetSelectedState();

	// 現在読み込まれているモデルのアニメーション一覧を取得
	void RefreshAvailableAnimations(entt::registry& registry);

	// 保存
	void Save(StudioContext& context);
	// 読み込み
	void Load(StudioContext& context);

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
	// 右側メニュー初期幅
	bool columnWidthInitialized = false;

	// D&D紐付けに失敗時のエラーメッセージ
	std::string attachError;

	// ノードの矩形保存
	std::unordered_map<int, std::pair<ImVec2, ImVec2>> nodeScreenRects;

	// Link起点StateID
	int pendingLinkFromId = -1;

};