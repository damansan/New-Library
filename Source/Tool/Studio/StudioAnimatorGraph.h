#pragma once
#include "IStudioAttachment.h"
#include "Tool/AnimatorGraph.h"
#include "Tool/NodeEditorCanvas.h"
#include "StudioConditionGraph.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>
#include <imgui.h>
#include <imgui_node_editor.h>

namespace ed = ax::NodeEditor;

class StudioAnimatorGraph : public IStudioAttachment
{
public:
	StudioAnimatorGraph() = default;
	~StudioAnimatorGraph() = default;

	void Initialize();
	void SetWindowId(int id) { windowId = id; }

	// ImGui描画
	void DrawGUI(entt::registry& registry, StudioContext& context) override;

private:
	// == 内部状態 == //
	NodeEditorCanvas canvas;
	AnimatorGraph graph;

	StudioConditionGraph conditionGraph; // Link1本分の条件編集用(別キャンバス)
	bool showConditionGraph = false;     // 現在どちらのキャンバスを表示中か
	
	int windowId = 0; // ウィンドウ番号

	std::vector<std::string> availableAnimations; // アニメーション一覧
	ImVec2 pendingCreatePos{};                    // 右クリック位置
	int pendingLinkFromId = -1;                   // Link起点StateID

	bool loaded = false;                 // 読み込み済みフラグ
	bool needsApplyPositions = false;    // 位置反映フラグ
	bool columnWidthInitialized = false; // 右側メニュー初期幅

	std::string attachError; // 紐づけエラーメッセージ
	
	std::unordered_map<int, std::pair<ImVec2, ImVec2>> nodeScreenRects; // ノードの矩形保存


	// == 各キャンバス描画 == /
	void DrawStateCanvas(StudioContext& context); // アニメーターキャンバス
	void DrawConditionCanvas();                   // 遷移条件キャンバス
	void DrawToolbar(StudioContext& context);     // 上側ツールバー 
	void DrawDropTarget(StudioContext& context);  // .animgraphのD&D受け口
	void TryAttachGraph(const std::string& path, StudioContext& context); // ドロップされたパスを紐づけ可能か検証する


	// == State描画 == //
	void DrawNodes();  // ノード描画
	void PushStateStyle(const AnimState& state); // Stateの種類に応じた見た目のスタイルを適用する
	void PopStateStyle(const AnimState& state);  // PushStateStyleで適用したスタイルを解除する 


	// == Link描画 == //
	void DrawLinks(); // Link全体の描画
	void CalcLinkPoints(const AnimLink& link, ImVec2& outStart, ImVec2& outEnd);     // リンク1本分の接続点を計算する
	void DrawLinkPins(const AnimLink& link, const ImVec2& start, const ImVec2& end); // 計算済みの座標にピンを配置する
	ImVec2 GetEdgePoint(const ImVec2& rectMin, const ImVec2& rectMax, const ImVec2& target); // 辺からリンクを伸ばす
	bool HasReverseLink(int fromId, int toId) const;                                         // 双方向のLinkがあるか確認する
	void DrawArrowHead(ImDrawList* drawList, const ImVec2& start, const ImVec2& end, ImU32 color);	// 矢印描画


	// == ノード作成 == //
	void DrawCreateNodeMenu();  // 右クリックメニュー
	void DrawNodeContextMenu(); // ノードの右クリックメニュー
	void UpdateLinkCreation();  // Link作成モード中の入力処理
	void DrawPendingLink();     // Link作成中のプレビュー線描画


	// == 選択状態とインスペクター == //
	void DrawInspector(AnimState* selectedState, AnimLink* selectedLink); // 右側メニューの振り分け
	void DrawStateInspector(AnimState* selectedState); // State選択時のInspector
	void DrawLinkInspector(AnimLink* selectedLink);    // Link選択時の Inspector
	AnimState* GetSelectedState(); // 選択中のStateを取得する
	AnimLink* GetSelectedLink();   // 選択中のLinkを取得する


	// == 表示用ヘルパー == //
	static void CenterNextItem(float totalWidth, float itemWidth);                   // ノード内で中央寄せにする
	void DrawDeleteButton(const char* label, const std::function<void()>& onDelete); // 共通の削除ボタン


	// == 動的ピンのID == //
	static ed::PinId GetOutPinId(int linkId) { return ed::PinId((int64_t)20000000 + (int64_t)linkId * 2); }
	static ed::PinId GetInPinId(int linkId) { return ed::PinId((int64_t)20000000 + (int64_t)linkId * 2 + 1); }


	// == その他 == //
	void RefreshAvailableAnimations(entt::registry& registry); // モデルが持つアニメーション一覧を更新する
	void Save(StudioContext& context); // 保存
	void Load(StudioContext& context); // 読み込み
};