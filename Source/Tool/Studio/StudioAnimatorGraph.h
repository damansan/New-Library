#pragma once
#include "IStudioAttachment.h"
#include "Engine/Data/AnimatorGraphData.h"
#include "StudioParentStateGraph.h"
#include "StudioChildStateGraph.h"
#include "StudioConditionGraph.h"

#include <vector>
#include <string>
#include <fstream>

namespace ed = ax::NodeEditor;

// 現在どのキャンバスを表示しているか
enum class AnimatorGraphView
{
	ParentGraph,
	ChildGraph,
	ConditionGraph,
};

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
	AnimatorGraphData graphData;

	StudioParentStateGraph parentStateGraph; // 親階層キャンバス
	StudioChildStateGraph childStateGraph;   // 子階層キャンバス
	StudioConditionGraph conditionGraph;     // 条件編集キャンバス

	AnimatorGraphView currentView = AnimatorGraphView::ParentGraph;  // 現在表示中のキャンバス
	AnimatorGraphView previousView = AnimatorGraphView::ParentGraph; // 条件グラフから戻る先
	ParentState* editingParent = nullptr; // 子階層編集中の対象

	int windowId = 0; // ウィンドウ番号
	std::vector<std::string> availableAnimations; // アニメーション一覧

	bool loaded = false;                 // 読み込み済みフラグ
	bool columnWidthInitialized = false; // 右側メニュー初期幅
	std::string attachError;             // 紐づけエラーメッセージ

	// == 各キャンバス描画 == //
	void DrawParentCanvas(StudioContext& context); // 親階層キャンバス
	void DrawChildCanvas();                        // 子階層キャンバス
	void DrawConditionCanvas();                    // 遷移条件キャンバス

	// == 画面遷移 == //
	void GoToCondition();                    // 条件グラフへ移動する(戻り先を記録する)
	void GoToChildGraph(ParentState* state); // 子階層グラフへ移動する

	// == 上側ツールバー == //
	void DrawToolbar(StudioContext& context);
	void DrawDropTarget(StudioContext& context); // .animgraphのD&D受け口
	void TryAttachGraph(const std::string& path, StudioContext& context); // ドロップされたパスを紐づけ可能か検証する

	// == その他 == //
	void RefreshAvailableAnimations(entt::registry& registry); // モデルが持つアニメーション一覧を更新する
	void Save(StudioContext& context); // .animgraphへの保存
	void Load(StudioContext& context); // .animgraphの読み込み

	// 新規保存先のパスを決定する
	std::string BuildNewSavePath(StudioContext& context);
};