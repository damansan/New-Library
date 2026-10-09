#pragma once
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
	// このクラス専用のノードエディタのコンテキスト
	NodeEditorCanvas canvas;

	// 編集中の条件グラフ(実体はStateMachineData側が持つ)
	ConditionGraph* graph = nullptr;

	// データの位置をノードに反映するか(対象を切り替えた直後の1フレームだけ)
	bool needsApplyPositions = true;
	// 次にキャンバスを描くときに、ノードエディタ側の選択を消すか
	bool clearSelectionRequested = false;
};