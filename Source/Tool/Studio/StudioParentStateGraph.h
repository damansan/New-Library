#include "Engine/Data/AnimatorGraphData.h"
#include "Tool/NodeEditor/NodeEditorCanvas.h"
#include "Tool/Studio/AnimatorGraphNavigation.h"
#include <unordered_map>
#include <functional>
#include <imgui.h>
#include <imgui_node_editor.h>

namespace ed = ax::NodeEditor;

class StudioParentStateGraph
{
public:
	StudioParentStateGraph() = default;
	~StudioParentStateGraph() = default;

	void Initialize();

	// ImGui描画
	void DrawGUI(AnimatorGraphData& graph);
	void DrawInspector();

	// 画面遷移が要求されていれば取り出す
	NavigationRequest ConsumeNavigationRequest(AnimLink*& outLink, ParentState*& outState);

	// 保存/読み込み直後など、座標を強制的に反映させたい時に呼ぶ
	void RequestApplyPositions() { needsApplyPositions = true; }
private:
	// == 内部状態 == //
	NodeEditorCanvas canvas;
	AnimatorGraphData* graph = nullptr;

	ImVec2 pendingCreatePos{};
	int pendingLinkFromId = -1;
	bool needsApplyPositions = false;

	std::unordered_map<int, std::pair<ImVec2, ImVec2>> stateScreenRects; // Stateの矩形保存

	ParentState* selectedState = nullptr;
	AnimLink* selectedLink = nullptr;

	NavigationRequest pendingNavigation = NavigationRequest::None; // 画面遷移の要求

	// == State描画 == //
	void DrawStates(); // State描画
	void PushStateStyle(const ParentState& state); // Stateの種類に応じた見た目のスタイルを適用する
	void PopStateStyle(const ParentState& state);  // PushStateStyleで適用したスタイルを解除する 

	// == Link描画 == //
	void DrawLinks(); // Link描画
	void CalcLinkPoints(const AnimLink& link, ImVec2& outStart, ImVec2& outEnd);			 // リンク1本分の接続点を計算する
	void DrawLinkPins(const AnimLink& link, const ImVec2& start, const ImVec2& end);		 // 計算済みの座標にピンを配置する
	ImVec2 GetEdgePoint(const ImVec2& rectMin, const ImVec2& rectMax, const ImVec2& target); // 辺からリンクを伸ばす
	bool HasReverseLink(int fromId, int toId) const;										 // 双方向のLinkがあるか確認する
	void DrawArrowHead(ImDrawList* drawList, const ImVec2& start, const ImVec2& end, ImU32 color); // 矢印描画

	// == 作成 == //
	void DrawCreateStateMenu();	 // 右クリックメニュー
	void DrawStateContextMenu(); // Link作成の起点選択(State右クリック)
	void UpdateLinkCreation();	 // Link作成モード中の入力処理
	void DrawPendingLink();		 // Link作成中のプレビュー線描画

	// == 選択状態とインスペクター == //
	void UpdateSelection(); // 選択状態を更新
	void DrawStateInspector(ParentState* state); // State選択時のInspector
	void DrawLinkInspector(AnimLink* link);      // Link選択時の Inspector

	// == 表示用ヘルパー == //
	static void CenterNextItem(float totalWidth, float itemWidth);					 // ノード内で中央寄せにする
	void DrawDeleteButton(const char* label, const std::function<void()>& onDelete); // 共通の削除ボタン

	// == 動的ピンID == //
	static ed::PinId GetPinId(int linkId, bool isOutput) { return ed::PinId((int64_t)20000000 + (int64_t)linkId * 2 + (isOutput ? 0 : 1)); }
};