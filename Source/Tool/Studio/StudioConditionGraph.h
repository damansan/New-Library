#pragma once
#include "Data/AnimatorGraphData.h"
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
	ConditionLink* selectedConditionLink = nullptr;

	// == 各ノード描画 == //
	void DrawReturnNode();    // リターンノード描画
	void DrawVariableNodes(); // 変数ノード描画
	void DrawOperatorNodes(); // 比較演算ノード描画
	void DrawGetterNodes();	  // ゲッターノード描画

	void DrawUnassignedGetterNode(ConditionGetterNode& g); // 未設定ゲッターノード
	void DrawAssignedGetterNode(ConditionGetterNode& g);   // 設定済みゲッターノード

	// == ピン接続 == //
	enum class PinRole // ピンの役割
	{
		VariableIn, VariableOut,
		OperatorA,  OperatorB,   OperatorOut,
		GetterOut,
		ReturnIn,
		Unknown
	};
	enum class NodeKind { Variable, Getter, Operator, Return, Unknown }; // ノードの種類

	static void ResolvePinRole(int64_t rawPinId, PinRole& outRole, int& outNodeId); // 生のピンID(int64_t)から、役割とノードIDを逆算する
	NodeKind GetNodeKind(int id) const; // 生のノードID(int)から、このLink内でどの種類のノードかを調べる

	void DrawLinkCreation();    // ピン接続(ドラッグ)の処理
	void DrawConditionLinks();  // 接続済みConditionLinkの描画

	// fromId/toIdからed::Linkを引くための、実際のピンIDを求める
	ed::PinId GetOutputPinIdOf(int nodeId, NodeKind kind) const;
	ed::PinId GetInputPinIdOf(int nodeId, NodeKind kind, bool toPinB) const;

	ValueType GetNodeValueType(NodeKind kind, int nodeId) const;        // ノードの値の型を取得する
	ValueType GetOutputValueType(PinRole outRole, int outNodeId) const; // 出力ピンの実質的な型を取得する
	static bool IsOrderCompareOp(CompareOp op);                         // 大小系演算子かを判定する

	// == ノードの作成 == //		
	void DrawCreateNodeMenu();               // 右クリックメニュー
	void CreateVariableNode(ValueType type); // 変数ノード作成
	void CreateOperatorNode(CompareOp op);   // 比較演算ノード作成
	void CreateGetterNode();                 // ゲッターノード作成

	// == ノードの削除 == //
	void DrawDeleteButton(const char* label, int targetId, const std::function<void(int)>& onDelete);
	void DeleteVariableNode(int id);  // 変数ノード削除
	void DeleteGetterNode(int id);    // ゲッターノード削除
	void DeleteOperatorNode(int id);  // 比較演算ノード削除
	void DeleteConditionLink(int id); // ConditionLink削除

	// == 選択状態 == //
	void UpdateSelection();
	ConditionLink* GetSelectedConditionLink();

	// == 表示用ヘルパー == //
	static void CenterNextItem(float totalWidth, float itemWidth); // ノード内で中央寄せにする
	static ImColor GetValueTypeColor(ValueType type);	           // 値の型ごとの表示色を返す
	static const char* GetValueTypeName(ValueType type);           // 値の型名を文字にして返す
	static const char* GetCompareOpSymbol(CompareOp op);	       // 比較演算子の表示用記号
	static CompareOp InvertCompareOp(CompareOp op);                // 比較演算子の向きを反転する

	// == ピンID == //
	static ed::PinId GetVariableInPinId(int nodeId) { return ed::PinId((int64_t)1000000 + (int64_t)nodeId * 2); }
	static ed::PinId GetVariableOutPinId(int nodeId) { return ed::PinId((int64_t)1000000 + (int64_t)nodeId * 2 + 1); }
	static ed::PinId GetOperatorPinA(int nodeId) { return ed::PinId((int64_t)2000000 + (int64_t)nodeId * 3); }
	static ed::PinId GetOperatorPinB(int nodeId) { return ed::PinId((int64_t)2000000 + (int64_t)nodeId * 3 + 1); }
	static ed::PinId GetOperatorOutPinId(int nodeId) { return ed::PinId((int64_t)2000000 + (int64_t)nodeId * 3 + 2); }
	static ed::PinId GetGetterOutPinId(int nodeId) { return ed::PinId((int64_t)3000000 + (int64_t)nodeId); }
	static ed::PinId GetReturnInPinId(int nodeId) { return ed::PinId((int64_t)4000000 + (int64_t)nodeId); }
};