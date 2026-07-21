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

private:
	NodeEditorCanvas canvas;        // このクラス専用のノードエディタコンテキスト
	AnimLink* targetLink = nullptr; // 現在編集中のLink
};