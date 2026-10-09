#include "StudioConditionCanvas.h"

void StudioConditionCanvas::Initialize()
{
	// ノードエディタのコンテキストを作る
	canvas.Initialize();
}

// 編集対象の条件グラフを設定する
void StudioConditionCanvas::SetTarget(ConditionGraph* target)
{
	graph = target;

	// 別の条件に切り替えたので、位置を反映し直し、前の条件の選択を消す
	needsApplyPositions = true;
	clearSelectionRequested = true;
}

// キャンバスを描く
void StudioConditionCanvas::Draw()
{
	if (!graph)
	{
		ImGui::TextDisabled("No link selected.");
		return;
	}

	canvas.Begin("ConditionCanvas");

	// 条件を切り替えた後は、ノードエディタ側に残った前の条件の選択を消す
	if (clearSelectionRequested)
	{
		ed::ClearSelection();
		clearSelectionRequested = false;
	}

	// (ノード・リンクの描画は、後の段階で足す)

	// 位置の反映は、切り替えた直後の1フレームだけ
	needsApplyPositions = false;

	canvas.End();
}

// インスペクターを描く
void StudioConditionCanvas::DrawInspector()
{
	ImGui::TextDisabled("No node selected.");
}