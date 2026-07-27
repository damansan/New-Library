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

	// ノード内で中央寄せにする
	static void CenterNextItem(float totalWidth, float itemWidth);
	// 値の型ごとの表示色を返す
	static ImColor GetValueTypeColor(ValueType type);

	// リターンノード描画
	void DrawReturnNode();
	// 変数ノード描画
	void DrawVariableNodes();
	// 比較演算ノード描画
	void DrawOperatorNodes();
	// ゲッターノード描画
	void DrawGetterNodes();


	// 右クリックメニュー
	void DrawCreateNodeMenu();
	// 変数ノード作成
	void CreateVariableNode(ValueType type);
	// 比較演算ノード作成
	void CreateOperatorNode(CompareOp op);
	// ゲッターノード作成
	void CreateGetterNode();


	// 比較演算子の表示用記号
	static const char* GetCompareOpSymbol(CompareOp op);

	ImVec2 pendingCreatePos{};

	static ed::PinId GetVariableInPinId(int nodeId) { return ed::PinId((int64_t)1000000 + (int64_t)nodeId * 2); }
	static ed::PinId GetVariableOutPinId(int nodeId) { return ed::PinId((int64_t)1000000 + (int64_t)nodeId * 2 + 1); }

	static ed::PinId GetOperatorPinA(int nodeId) { return ed::PinId((int64_t)2000000 + (int64_t)nodeId * 3); }
	static ed::PinId GetOperatorPinB(int nodeId) { return ed::PinId((int64_t)2000000 + (int64_t)nodeId * 3 + 1); }
	static ed::PinId GetOperatorOutPinId(int nodeId) { return ed::PinId((int64_t)2000000 + (int64_t)nodeId * 3 + 2); }

	static ed::PinId GetGetterOutPinId(int nodeId) { return ed::PinId((int64_t)3000000 + (int64_t)nodeId); }

	bool needsApplyPositions = false;


	// ノード選択状態を更新
	void UpdateSelection();

	ConditionVariableNode* selectedVariable = nullptr;
	ConditionGetterNode* selectedGetter = nullptr;
	ConditionOperatorNode* selectedOperator = nullptr;

};