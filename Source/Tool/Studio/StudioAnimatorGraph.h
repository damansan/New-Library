#pragma once
#include "IStudioAttachment.h"
#include "Tool/AnimatorGraph.h"
#include "Tool/NodeEditorCanvas.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <imgui.h>
#include <imgui_node_editor.h>

#include "StudioConditionGraph.h"

namespace ed = ax::NodeEditor;

class StudioAnimatorGraph : public IStudioAttachment
{
public:
	StudioAnimatorGraph() = default;
	~StudioAnimatorGraph() = default;

	void Initialize();
	void SetWindowId(int id) { windowId = id; }

	// ImGui描画
	void DrawGUI(entt::registry& registry, StudioContext& context) override;

private:

	// アニメーターキャンバス
	void DrawStateCanvas(StudioContext& context);
	// 遷移条件キャンバス
	void DrawConditionCanvas();

	// 上側ツールバー
	void DrawToolbar(StudioContext& context);

	// .animgraphのD&D受け口
	void DrawDropTarget(StudioContext& context);
	void TryAttachGraph(const std::string& path, StudioContext& context);

	// State描画
	void DrawNodes();
	void PushStateStyle(const AnimState& state);
	void PopStateStyle(const AnimState& state);

	// Link描画
	void DrawLinks();
	// リンク1本分の接続点を計算する
	void CalcLinkPoints(const AnimLink& link, ImVec2& outStart, ImVec2& outEnd);
	// 計算済みの座標にピンを配置する
	void DrawLinkPins(const AnimLink& link, const ImVec2& start, const ImVec2& end);
	// 辺からリンクを伸ばす
	ImVec2 GetEdgePoint(const ImVec2& rectMin, const ImVec2& rectMax, const ImVec2& target);
	// 双方向のLinkがあるか確認する
	bool HasReverseLink(int fromId, int toId) const;
	// 矢印描画
	void DrawArrowHead(ImDrawList* drawList, const ImVec2& start, const ImVec2& end, ImU32 color);
	// LinkID
	static ed::PinId GetOutPinId(int linkId) { return ed::PinId((int64_t)20000000 + (int64_t)linkId * 2); }
	static ed::PinId GetInPinId(int linkId) { return ed::PinId((int64_t)20000000 + (int64_t)linkId * 2 + 1); }
	// 右クリックメニュー
	void DrawCreateNodeMenu();
	void DrawNodeContextMenu();
	void UpdateLinkCreation();
	void DrawPendingLink();

	// 右側メニュー
	void DrawInspector(AnimState* selectedState, AnimLink* selectedLink);
	void DrawStateInspector(AnimState* selectedState);
	void DrawLinkInspector(AnimLink* selectedLink);
	AnimState* GetSelectedState();
	AnimLink* GetSelectedLink();

	void RefreshAvailableAnimations(entt::registry& registry);

	// 保存・読み込み
	void Save(StudioContext& context);
	void Load(StudioContext& context);

	NodeEditorCanvas canvas;
	AnimatorGraph graph;
	
	StudioConditionGraph conditionGraph; // Link1本分の条件編集用(別キャンバス)
	bool showConditionGraph = false;     // 現在どちらのキャンバスを表示中か

	int windowId = 0;

	// アニメーション一覧
	std::vector<std::string> availableAnimations;
	// 右クリック位置
	ImVec2 pendingCreatePos{};

	// 読み込み済みフラグ
	bool loaded = false;
	// 位置反映フラグ
	bool needsApplyPositions = false;
	// 右側メニュー初期幅
	bool columnWidthInitialized = false;

	// 紐づけエラーメッセージ
	std::string attachError;

	// ノードの矩形保存
	std::unordered_map<int, std::pair<ImVec2, ImVec2>> nodeScreenRects;

	// Link起点StateID
	int pendingLinkFromId = -1;
};