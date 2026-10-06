#pragma once
#include "Engine/Data/ConditionGraphData.h"
#include "Tool/NodeEditor/NodeEditorCanvas.h"
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

	// 編集対象の条件グラフを設定する
	void SetTarget(ConditionGraph* condition);

	// ImGui描画
	void DrawGUI();
	// 右側メニュー
	void DrawInspector();

private:
	// == 内部状態 == //
	NodeEditorCanvas canvas;        // このクラス専用のノードエディタコンテキスト
	ConditionGraph* targetCondition = nullptr; // 現在編集中の条件グラフ
	ImVec2 pendingCreatePos{};
	bool needsApplyPositions = false;

	ConditionVariableNode* selectedVariable = nullptr;
	ConditionGetterNode* selectedGetter = nullptr;
	ConditionOperatorNode* selectedOperator = nullptr;
	ConditionLogicNode* selectedLogic = nullptr;
	ConditionNotNode* selectedNot = nullptr;
	ConditionLink* selectedConditionLink = nullptr;

	// == 各ノード描画 == //
	void DrawReturnNode();    // リターンノード描画
	void DrawVariableNodes(); // 変数ノード描画
	void DrawOperatorNodes(); // 比較演算ノード描画
	void DrawLogicNodes();    // 論理演算ノード描画
	void DrawGetterNodes();	  // ゲッターノード描画
	void DrawNotNodes();      // 否定ノード描画

	void DrawUnassignedGetterNode(ConditionGetterNode& g); // 未設定ゲッターノード
	void DrawAssignedGetterNode(ConditionGetterNode& g);   // 設定済みゲッターノード

	// == ピン接続 == //
	enum class PinRole // ピンの役割
	{
		VariableIn, VariableOut,
		OperatorA, OperatorB, OperatorOut,
		LogicA, LogicB, LogicOut,
		NotIn, NotOut,
		GetterOut,
		ReturnIn,
		Unknown
	};
	enum class NodeKind { Variable, Getter, Operator, Logic, Not, Return, Unknown }; // ノードの種類

	// ピンID計算用のベースオフセット
	static constexpr int64_t VariableOffset = 1000000;
	static constexpr int64_t OperatorOffset = 2000000;
	static constexpr int64_t GetterOffset = 3000000;
	static constexpr int64_t ReturnOffset = 4000000;
	static constexpr int64_t LogicOffset = 5000000;
	static constexpr int64_t NotOffset = 6000000;

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
	void CreateLogicNode(LogicOp op);        // 論理演算ノード作成
	void CreateNotNode();                    // 否定ノード作成
	void CreateGetterNode();                 // ゲッターノード作成

	// == ノードの削除 == //
	void DrawDeleteButton(const char* label, int targetId, const std::function<void(int)>& onDelete);
	void DeleteVariableNode(int id);  // 変数ノード削除
	void DeleteGetterNode(int id);    // ゲッターノード削除
	void DeleteOperatorNode(int id);  // 比較演算ノード削除
	void DeleteLogicNode(int id);     // 論理演算ノード削除
	void DeleteNotNode(int id);       // 否定ノード削除
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
	static const char* GetLogicOpSymbol(LogicOp op);               // 論理演算子の表示用記号

	// == ピンID == //
	static ed::PinId GetVariableInPinId  (int nodeId) { return ed::PinId(VariableOffset + (int64_t)nodeId * 2); }
	static ed::PinId GetVariableOutPinId (int nodeId) { return ed::PinId(VariableOffset + (int64_t)nodeId * 2 + 1); }
	static ed::PinId GetOperatorPinA     (int nodeId) { return ed::PinId(OperatorOffset + (int64_t)nodeId * 3); }
	static ed::PinId GetOperatorPinB     (int nodeId) { return ed::PinId(OperatorOffset + (int64_t)nodeId * 3 + 1); }
	static ed::PinId GetOperatorOutPinId (int nodeId) { return ed::PinId(OperatorOffset + (int64_t)nodeId * 3 + 2); }
	static ed::PinId GetGetterOutPinId   (int nodeId) { return ed::PinId(GetterOffset   + (int64_t)nodeId); }
	static ed::PinId GetReturnInPinId    (int nodeId) { return ed::PinId(ReturnOffset   + (int64_t)nodeId); }
	static ed::PinId GetLogicPinA        (int nodeId) { return ed::PinId(LogicOffset    + (int64_t)nodeId * 3); }
	static ed::PinId GetLogicPinB        (int nodeId) { return ed::PinId(LogicOffset    + (int64_t)nodeId * 3 + 1); }
	static ed::PinId GetLogicOutPinId    (int nodeId) { return ed::PinId(LogicOffset    + (int64_t)nodeId * 3 + 2); }
	static ed::PinId GetNotInPinId       (int nodeId) { return ed::PinId(NotOffset      + (int64_t)nodeId * 2); }
	static ed::PinId GetNotOutPinId      (int nodeId) { return ed::PinId(NotOffset      + (int64_t)nodeId * 2 + 1); }
};