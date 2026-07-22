#pragma once
#include "Tool/AnimatorGraph.h"
#include "Tool/NodeEditorCanvas.h"
#include <imgui.h>
#include <imgui_node_editor.h>

namespace ed = ax::NodeEditor;

class StudioConditionGraph
{
public:
	StudioConditionGraph() = default;
	~StudioConditionGraph() = default;

	void Initialize();

	// 編集対象のLinkを設定する
	void SetTarget(AnimLink* link);

	// ImGui描画
	void DrawGUI();
	// 右側メニュー
	void DrawInspector();
private:
	NodeEditorCanvas canvas;        // このクラス専用のノードエディタコンテキスト
	AnimLink* targetLink = nullptr; // 現在編集中のLink

	// リターンノード描画
	void DrawReturnNode();
	// 変数ノード描画
	void DrawVariableNodes();

	// 右クリックメニュー
	void DrawCreateNodeMenu();

	ImVec2 pendingCreatePos{};

	static ed::PinId GetVariableInPinId(int nodeId) { return ed::PinId((int64_t)1000000 + (int64_t)nodeId * 2); }
	static ed::PinId GetVariableOutPinId(int nodeId) { return ed::PinId((int64_t)1000000 + (int64_t)nodeId * 2 + 1); }

	bool needsApplyPositions = false;

};