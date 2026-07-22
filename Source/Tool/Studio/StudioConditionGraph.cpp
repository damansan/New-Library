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
	DrawVariableNodes();

	DrawCreateNodeMenu();

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

// 変数ノード描画
void StudioConditionGraph::DrawVariableNodes()
{
	for (auto& v : targetLink->condition.variableNodes)
	{
		ed::NodeId nodeId(v.id);

		if (needsApplyPositions)
		{
			ed::SetNodePosition(nodeId, ImVec2(v.posX, v.posY));
		}

		ed::BeginNode(nodeId);

		ImGui::Text("Variable");

		ed::BeginPin(GetVariableInPinId(v.id), ed::PinKind::Input);
		ImGui::Text("in");
		ed::EndPin();

		ImGui::SameLine();

		ed::BeginPin(GetVariableOutPinId(v.id), ed::PinKind::Output);
		ImGui::Text("out");
		ed::EndPin();

		ed::EndNode();

		ImVec2 currentPos = ed::GetNodePosition(nodeId);
		v.posX = currentPos.x;
		v.posY = currentPos.y;
	}
}

// 右クリックメニュー
void StudioConditionGraph::DrawCreateNodeMenu()
{
	ed::Suspend();
	if (ed::ShowBackgroundContextMenu())
	{
		pendingCreatePos = ImGui::GetMousePos();
		ImGui::OpenPopup("CreateConditionNode");
	}
	if (ImGui::BeginPopup("CreateConditionNode"))
	{
		if (ImGui::MenuItem("Add Variable"))
		{
			ImVec2 canvasPos = ed::ScreenToCanvas(pendingCreatePos);

			ConditionVariableNode v;
			v.id = targetLink->condition.GenerateId();
			v.posX = canvasPos.x;
			v.posY = canvasPos.y;

			targetLink->condition.variableNodes.push_back(v);

			ed::SetNodePosition(ed::NodeId(v.id), canvasPos);
		}
		ImGui::EndPopup();
	}
	ed::Resume();
}