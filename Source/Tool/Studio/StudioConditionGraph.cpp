#include "StudioConditionGraph.h"

void StudioConditionGraph::Initialize()
{
	canvas.Initialize();
}

void StudioConditionGraph::SetTarget(AnimLink* link)
{
	targetLink = link;
}

void StudioConditionGraph::DrawGUI()
{
	if (!targetLink)
	{
		ImGui::TextDisabled("No link selected.");
		return;
	}

	canvas.Begin("ConditionCanvas");

	// 今のところ、何も描画しない(空のキャンバス)

	canvas.End();
}

// 右側メニュー
void StudioConditionGraph::DrawInspector()
{
	ImGui::TextDisabled("No node selected.");

}
