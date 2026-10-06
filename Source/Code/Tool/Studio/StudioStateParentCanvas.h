#pragma once
#include <unordered_map>
#include <utility>
#include <vector>

#include "Engine/Data/StateMachineData.h"
#include "Tool/NodeEditor/NodeEditorCanvas.h"

// Stateグラフの親のキャンバス
class StudioStateParentCanvas
{
public:
	void Initialize();

	// キャンバスを描く
	void Draw(StateMachineData& data);
	// 選択している物を削除
	void DeleteSelection(StateMachineData& data);
	// 読み込み・新規作成のときに、選択などを戻す
	void Reset();

	// 選択中のノードとLink
	int GetSelectedNodeKey() const { return selectedNodeKey; }
	int GetSelectedLinkId() const { return selectedLinkId; }

private:
	// --- 描画 --- //

	// ノード描画
	void DrawEntryNode();
	void DrawStateNodes();

	// リンク描画
	void DrawLinks();
	// 作成中のリンク描画
	void DrawPendingLink();

	// 背景右クリックのメニュー
	void DrawBackgroundMenu();
	// ノード右クリックのメニュー
	void DrawNodeMenu();

	// --- 入力 --- //

	// リンク作成中の入力
	void UpdateLinkCreation();
	// 選択の更新
	void UpdateSelection();


	// --- リンクの計算 --- //

	// fromKeyからtoStateIdへのリンクがあるか
	bool HasLink(int fromKey, int toStateId) const;
	// 線の始点と終点を求める
	bool CalcLinkPoints(int fromKey, int toStateId, int lane, ImVec2& outStart, ImVec2& outEnd) const;
	// 矩形の中心からtargetへ伸ばした線が、矩形の縁と交わる点
	static ImVec2 GetEdgePoint(const ImVec2& rectMin, const ImVec2& rectMax, const ImVec2& target);
	// 矢印描画
	static void DrawArrow(ImDrawList* drawList, const ImVec2& start, const ImVec2& end, ImU32 color, float thickness);
	// 点と線分の距離
	static float DistanceToSegment(const ImVec2& p, const ImVec2& a, const ImVec2& b);


	// --- ノードのID --- //

	// EntryはデータのIDと被らない範囲の値を使う
	static constexpr int64_t kEntryNodeId = 1ll << 40;
	// ノードのキー <-> ノードエディタのID
	static ed::NodeId ToNodeId(int key);
	static int ToNodeKey(ed::NodeId nodeId);


	// --- データ --- //

	StateMachineData* data = nullptr;

	// --- キャンバスの状態 --- //

	NodeEditorCanvas canvas;
	// データの位置をノードに反映するか
	bool needsApplyPositions = true;
	// ノードの矩形
	std::unordered_map<int, std::pair<ImVec2, ImVec2>> nodeRects;

	// 描いたリンクの線
	struct LinkSegment
	{
		int linkId;
		ImVec2 start;
		ImVec2 end;
	};
	std::vector<LinkSegment> linkSegments;

	// 選択中のノードとLink
	int selectedNodeKey = -1;
	int selectedLinkId = -1;

	// リンクの作成中の遷移元
	int pendingFromKey = -1;

	// 背景の右クリックの位置
	ImVec2 pendingCreatePos{};
	// ノードの右クリックの対象
	int contextNodeKey = -1;
};