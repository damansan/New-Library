#pragma once
#include "Tool/AnimatorGraph.h"
#include "Tool/NodeEditorCanvas.h"
#include <imgui.h>
#include <functional>
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
	// == 内部状態 == //
	NodeEditorCanvas canvas;        // このクラス専用のノードエディタコンテキスト
	AnimLink* targetLink = nullptr; // 現在編集中のLink
	ImVec2 pendingCreatePos{};
	bool needsApplyPositions = false;

	ConditionVariableNode* selectedVariable = nullptr;
	ConditionGetterNode* selectedGetter = nullptr;
	ConditionOperatorNode* selectedOperator = nullptr;

	// == 各ノード描画 == //
	void DrawReturnNode();    // リターンノード描画
	void DrawVariableNodes(); // 変数ノード描画
	void DrawOperatorNodes(); // 比較演算ノード描画
	void DrawGetterNodes();	  // ゲッターノード描画

	void DrawUnassignedGetterNode(ConditionGetterNode& g); // 未設定ゲッターノード
	void DrawAssignedGetterNode(ConditionGetterNode& g);   // 設定済みゲッターノード

	// == ノードの作成 == //		
	void DrawCreateNodeMenu();               // 右クリックメニュー
	void CreateVariableNode(ValueType type); // 変数ノード作成
	void CreateOperatorNode(CompareOp op);   // 比較演算ノード作成
	void CreateGetterNode();                 // ゲッターノード作成

	// == ノードの削除 == //
	void DrawDeleteButton(const char* label, int targetId, const std::function<void(int)>& onDelete);
	void DeleteVariableNode(int id);
	void DeleteGetterNode(int id);
	void DeleteOperatorNode(int id);

	// == 選択状態 == //
	void UpdateSelection();

	// == 表示用ヘルパー == //
	static void CenterNextItem(float totalWidth, float itemWidth); // ノード内で中央寄せにする
	static ImColor GetValueTypeColor(ValueType type);	           // 値の型ごとの表示色を返す
	static const char* GetValueTypeName(ValueType type);           // 値の型名を文字にして返す
	static const char* GetCompareOpSymbol(CompareOp op);	       // 比較演算子の表示用記号

	// == ピンID == //
	static ed::PinId GetVariableInPinId(int nodeId)  { return ed::PinId((int64_t)1000000 + (int64_t)nodeId * 2);     }
	static ed::PinId GetVariableOutPinId(int nodeId) { return ed::PinId((int64_t)1000000 + (int64_t)nodeId * 2 + 1); }
	static ed::PinId GetOperatorPinA(int nodeId)     { return ed::PinId((int64_t)2000000 + (int64_t)nodeId * 3);     }
	static ed::PinId GetOperatorPinB(int nodeId)     { return ed::PinId((int64_t)2000000 + (int64_t)nodeId * 3 + 1); }
	static ed::PinId GetOperatorOutPinId(int nodeId) { return ed::PinId((int64_t)2000000 + (int64_t)nodeId * 3 + 2); }
	static ed::PinId GetGetterOutPinId(int nodeId)   { return ed::PinId((int64_t)3000000 + (int64_t)nodeId);         }
};