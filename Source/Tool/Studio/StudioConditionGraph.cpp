#include "StudioConditionGraph.h"

void StudioConditionGraph::Initialize()
{
	canvas.Initialize();
}

void StudioConditionGraph::SetTarget(AnimLink* link)
{
	targetLink = link;
	needsApplyPositions = true;
}

void StudioConditionGraph::DrawGUI()
{
	if (!targetLink)
	{
		ImGui::TextDisabled("No link selected.");
		return;
	}

	canvas.Begin("ConditionCanvas");

	DrawReturnNode();

	canvas.End();

	needsApplyPositions = false;
}

// 右側メニュー
void StudioConditionGraph::DrawInspector()
{
	ImGui::TextDisabled("No node selected.");

}

// リターンノード描画
void StudioConditionGraph::DrawReturnNode()
{
	ConditionReturnNode& returnNode = targetLink->condition.returnNode;
	ed::NodeId nodeId(returnNode.id);

	if (needsApplyPositions)
	{
		ed::SetNodePosition(nodeId, ImVec2(returnNode.posX, returnNode.posY));
	}

	// Visual Studioのreturn相当の紫色
	const ImColor bgColor(197, 134, 192, 255);
	const ImColor borderColor(140, 90, 137, 255);

	ed::PushStyleColor(ed::StyleColor_NodeBg, bgColor);
	ed::PushStyleColor(ed::StyleColor_NodeBorder, borderColor);
	ed::PushStyleVar(ed::StyleVar_NodeRounding, 4.0f);

	ed::BeginNode(nodeId);

	const float totalWidth = 100.0f;
	float textWidth = ImGui::CalcTextSize("Return").x;
	float padding = (totalWidth - textWidth) * 0.5f;

	if (padding > 0.0f)
	{
		ImGui::Dummy(ImVec2(padding, 0.0f));
		ImGui::SameLine(0.0f, 0.0f);
	}

	ImGui::Text("Return");

	if (padding > 0.0f)
	{
		ImGui::SameLine(0.0f, 0.0f);
		ImGui::Dummy(ImVec2(padding, 0.0f));
	}

	ed::EndNode();

	ImVec2 currentPos = ed::GetNodePosition(nodeId);
	returnNode.posX = currentPos.x;
	returnNode.posY = currentPos.y;

	ed::PopStyleColor(2);
	ed::PopStyleVar(1);
}
