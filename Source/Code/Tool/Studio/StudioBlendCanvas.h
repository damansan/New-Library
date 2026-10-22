#pragma once
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <imgui.h>

#include "Engine/Data/StateMachineData.h"
#include "Tool/NodeEditor/NodeEditorCanvas.h"

// Blend の中のグラフ
class StudioBlendCanvas
{
public:
	StudioBlendCanvas(int stateId, int blendId) : stateId(stateId), blendId(blendId) {}

	void Initialize();

	// どの State の、どの Blend か
	int GetStateId() const { return stateId; }
	int GetBlendId() const { return blendId; }

	// 編集する Blend を探す
	static BlendNode* FindBlendNode(StateMachineData& data, int stateId, int blendId);

	// キャンバスとインスペクターを描く
	void Draw(BlendNode& blend, const std::vector<std::string>& animations);

	// Output への線の条件を編集してほしい依頼
	int ConsumeEditConditionRequest() { int id = editConditionRequest; editConditionRequest = -1; return id; }

private:
	// 選んでいる物の種類
	enum class SelectionKind
	{
		None,
		Node,       // Output か材料
		Pair,       // ペアの線
		OutputLink, // Output への線
	};

	// --- 描画 --- //

	// ノード
	void DrawOutputNode();
	void DrawSourceNodes();

	// 線
	void DrawPairLinks();
	void DrawOutputLinks();
	void DrawPendingLink();

	// メニュー
	void DrawBackgroundMenu(); // 背景
	void DrawNodeMenu();       // ノード


	// --- 入力 --- //

	// リンクの作成中の入力
	void UpdateLinkCreation();
	// 選択の更新
	void UpdateSelection();
	// 選んでいる物を削除
	void DeleteSelection();
	// 選択を外す
	void ClearSelection();

	// その位置にあるペアを探す
	int HitPair(const ImVec2& canvasPos) const;


	// --- インスペクター --- //

	void DrawInspector(const std::vector<std::string>& animations);
	void DrawOutputInspector();
	void DrawSourceInspector(BlendSourceNode& source, const std::vector<std::string>& animations);
	void DrawPairInspector(BlendPairLink& pair);
	void DrawOutputLinkInspector(BlendOutputLink& link);

	// 材料のアニメーション一覧
	void DrawAnimationList(BlendSourceNode& source, const std::vector<std::string>& animations);
	// ペアの比率を読む変数を選ぶ
	void DrawRateVariablePicker(BlendPairLink& pair);
	// Output への線の一覧
	void DrawOutputLinkList();


	// --- 名前・位置 --- //

	// 材料の表示名
	std::string GetSourceName(int sourceId) const;
	// 線の元の表示名
	std::string GetFromName(int fromId) const;

	// 線の始点
	bool CalcLinkStart(int fromId, const ImVec2& toward, ImVec2& outStart) const;


	// --- データ --- //

	int stateId = -1;
	int blendId = -1;

	// 描画中だけ、編集する Blend を指す
	BlendNode* blend = nullptr;
	BlendGraph* graph = nullptr;


	// --- キャンバスの状態 --- //

	NodeEditorCanvas canvas;
	// データの位置をノードに反映するか
	bool needsApplyPositions = true;
	// 次にキャンバスを描くときに、ノードエディタ側の選択を消すか
	bool clearSelectionRequested = false;

	// ノードの矩形
	std::unordered_map<int, std::pair<ImVec2, ImVec2>> nodeRects;
	// ペアの真ん中の点
	std::unordered_map<int, ImVec2> pairMidpoints;

	// 描いた線
	struct LinkSegment
	{
		int id;
		ImVec2 start;
		ImVec2 end;
	};
	std::vector<LinkSegment> pairSegments;
	std::vector<LinkSegment> outputSegments;

	// 選んでいる物
	SelectionKind selectedKind = SelectionKind::None;
	int selectedId = -1;

	// リンクの作成中の元
	int pendingLinkFromId = -1;

	// 右クリックの位置と対象
	ImVec2 pendingCreatePos{};
	int contextNodeId = -1;
	int contextPairId = -1;

	bool columnWidthInitialized = false;

	// 条件を編集してほしい Output への線
	int editConditionRequest = -1;


	// --- インスペクターの状態 --- //

	// 材料の名前の入力欄
	int nameEditingSourceId = -1;
	char nameBuffer[128] = {};
	// アニメーションの一覧
	char animationFilter[64] = {};
	int animationListSourceId = -1;
	bool scrollToCurrentAnimation = false;

	// 比率の変数の一覧
	int rateInspectorPairId = -1;
	std::string browseComponent;
	bool scrollToCurrentField = false;
};