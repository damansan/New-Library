#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <imgui.h>

#include "Engine/Data/ConditionGraphData.h"
#include "Tool/NodeEditor/NodeEditorCanvas.h"

// 条件グラフの編集キャンバス
class StudioConditionCanvas
{
public:
	void Initialize();

	// 編集対象の条件グラフを設定する(nullptrで解除)
	void SetTarget(ConditionGraph* target);

	// キャンバスを描く
	void Draw();
	// インスペクターを描く
	void DrawInspector();

private:

	// --- ノードの描画 --- //

	// リターンノード描画
	void DrawReturnNode();
	// 変数ノード描画
	void DrawVariableNodes();
	// ゲッターノード描画
	void DrawGetterNodes();
	// 比較演算ノード描画
	void DrawOperatorNodes();
	// 論理演算ノード描画
	void DrawLogicNodes();
	// 否定ノード描画
	void DrawNotNodes();

	// リンク描画
	void DrawLinks();
	
	// 背景右クリックのノード作成メニュー
	void DrawCreateMenu();

	// ピンからドラッグしてリンクを作る
	void UpdateLinkCreation();
	// ノードエディタの選択を、こちらの選択に写す
	void UpdateSelection();
	// 選択している物を削除
	void DeleteSelection();


	// --- インスペクター --- //

	// リターンノードの設定
	void DrawReturnInspector();
	// 変数ノードの設定
	void DrawVariableInspector(ConditionVariableNode& node);
	// ゲッターノードの設定
	void DrawGetterInspector(ConditionGetterNode& node);
	// 比較演算ノードの設定
	void DrawOperatorInspector(ConditionOperatorNode& node);
	// 論理演算ノードの設定
	void DrawLogicInspector(ConditionLogicNode& node);
	// 否定ノードの設定
	void DrawNotInspector();

	// リンクの設定
	void DrawLinkInspector(const ConditionLink& link);

	// 表示用ノードの名前
	std::string GetNodeName(int nodeId) const;


	// --- 位置 --- //

	// 対象を切り替えた直後だけ、データの位置をノードに反映する
	void ApplyPosition(int nodeId, float x, float y) const;
	// ノードを動かしたら、データの位置に書き戻す
	static void ReadPosition(int nodeId, float& x, float& y);
	// 背景を右クリックした位置
	ImVec2 pendingCreatePos{};

	// --- ピンのID --- //

		// ノードの中でのピンの位置
	enum PinSlot : uint64_t
	{
		kPinOut = 1, // 出力
		kPinInA = 2, // 入力A(入力が1つのノードもA)
		kPinInB = 3, // 入力B
	};
	// ノードのIDと重ならないように付ける印
	static constexpr uint64_t kPinIdTag = 1ull << 40;

	// ノードのIDと位置 <-> ピンのID
	static ed::PinId ToPinId(int nodeId, PinSlot slot);
	static bool FromPinId(ed::PinId pinId, int& outNodeId, PinSlot& outSlot);


	// このクラス専用のノードエディタのコンテキスト
	NodeEditorCanvas canvas;
	// 編集中の条件グラフ(実体はStateMachineData側が持つ)
	ConditionGraph* graph = nullptr;

	// データの位置をノードに反映するか(対象を切り替えた直後の1フレームだけ)
	bool needsApplyPositions = true;
	// 次にキャンバスを描くときに、ノードエディタ側の選択を消すか
	bool clearSelectionRequested = false;


	// 選択中のノード(複数)とリンク(1本)。ノードとリンクは同時には選ばない
	std::vector<int> selectedNodeIds;
	int selectedLinkId = -1;

	// ゲッターのインスペクターの状態
	int getterInspectorNodeId = -1; // 表示しているゲッター
	std::string browseComponent;    // 一覧で開いているコンポーネント
	bool scrollToCurrent = false;   // 今の設定の位置までスクロールするか
};