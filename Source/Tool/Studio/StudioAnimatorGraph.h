#pragma once
#include "IStudioAttachment.h"
#include "Data/AnimatorGraphData.h"
#include "StudioParentStateGraph.h"
#include "StudioConditionGraph.h"

#include <vector>
#include <string>
#include <fstream>

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
	AnimatorGraphData graphData;

	StudioParentStateGraph parentStateGraph; // 親階層キャンバス
	StudioConditionGraph conditionGraph;     // 条件編集キャンバス
	bool showConditionGraph = false;         // 現在どちらのキャンバスを表示中か

	int windowId = 0; // ウィンドウ番号
	std::vector<std::string> availableAnimations; // アニメーション一覧

	bool loaded = false;                 // 読み込み済みフラグ
	bool columnWidthInitialized = false; // 右側メニュー初期幅
	std::string attachError;             // 紐づけエラーメッセージ

	// == 各キャンバス描画 == //
	void DrawStateCanvas();      // 親階層(State)キャンバス
	void DrawConditionCanvas();  // 遷移条件キャンバス


	// == 上側ツールバー == //
	void DrawToolbar(StudioContext& context);
	void DrawDropTarget(StudioContext& context); // .animgraphのD&D受け口
	void TryAttachGraph(const std::string& path, StudioContext& context); // ドロップされたパスを紐づけ可能か検証する

	// == その他 == //
	void RefreshAvailableAnimations(entt::registry& registry); // モデルが持つアニメーション一覧を更新する
	void Save(StudioContext& context); // .animgraphへの保存
	void Load(StudioContext& context); // .animgraphの読み込み
};