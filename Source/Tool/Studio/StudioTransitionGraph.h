#pragma once
#include "Engine/Data/AnimatorGraphData.h"
#include "Tool/NodeEditor/NodeEditorCanvas.h"
#include <unordered_map>
#include <functional>
#include <imgui.h>
#include <imgui_node_editor.h>

namespace ed = ax::NodeEditor;

class StudioTransitionGraph
{
public:
	StudioTransitionGraph() = default;
	~StudioTransitionGraph() = default;

	void Initialize();

	// ImGui描画
	void DrawGUI(AnimatorGraphData& graph);
	void DrawInspector();

	// 条件式編集画面への遷移要求
	bool ConsumeConditionEditRequest(TransitionLink*& outLink);

	// 保存/読み込み直後など、座標を強制反映したい時に呼ぶ
	void RequestApplyPositions() { needsApplyPositions = true; }

private:
	// == 状態 == //
	NodeEditorCanvas canvas;
	AnimatorGraphData* graph = nullptr;

	int pendingLinkFromId = -1;
	bool needsApplyPositions = false;

	std::unordered_map<int, std::pair<ImVec2, ImVec2>> stateScreenRects; // Stateの矩形を保存(parentGraph.states由来)

	ParentState* selectedState = nullptr;     // 選択中のState
	TransitionLink* selectedLink = nullptr;   // 選択中のLink

	bool pendingConditionEdit = false; // 条件式編集画面への遷移要求フラグ

	// == State描画 == //
	void DrawStates();

	// == Link描画 == //
	void DrawLinks();
	void CalcLinkPoints(const TransitionLink& link, ImVec2& outStart, ImVec2& outEnd);
	void DrawLinkPins(const TransitionLink& link, const ImVec2& start, const ImVec2& end);
	ImVec2 GetEdgePoint(const ImVec2& rectMin, const ImVec2& rectMax, const ImVec2& target);
	bool HasReverseLink(int fromId, int toId) const;
	void DrawArrowHead(ImDrawList* drawList, const ImVec2& start, const ImVec2& end, ImU32 color);

	// == 作成 == //
	void DrawStateContextMenu(); // Link作成の起点選択
	void UpdateLinkCreation();   // Link作成モードの入力処理
	void DrawPendingLink();      // Link作成中のプレビュー描画

	// == 選択とインスペクター == //
	void UpdateSelection();
	void DrawLinkInspector(TransitionLink* link);

	// == 表示用ヘルパー == //
	static void CenterNextItem(float totalWidth, float itemWidth);
	void DrawDeleteButton(const char* label, const std::function<void()>& onDelete);

	// == PinID == //
	static ed::PinId GetPinId(int linkId, bool isOutput) { return ed::PinId((int64_t)30000000 + (int64_t)linkId * 2 + (isOutput ? 0 : 1)); }
};