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
	DrawOperatorNodes();
	DrawGetterNodes();

	DrawCreateNodeMenu();

	UpdateSelection();

	canvas.End();

	needsApplyPositions = false;
}

// 右側メニュー
void StudioConditionGraph::DrawInspector()
{
	if (selectedVariable)
	{
		ImGui::Text("Variable");
		ImGui::TextDisabled("(Editing UI not implemented yet)");
		return;
	}

	if (selectedGetter)
	{
		ImGui::Text("Getter");
		ImGui::TextDisabled("(Editing UI not implemented yet)");
		return;
	}

	if (selectedOperator)
	{
		ImGui::Text("Operator");
		ImGui::TextDisabled("(Editing UI not implemented yet)");
		return;
	}

	ImGui::TextDisabled("No node selected.");
}

// ノード内で中央寄せにする
void StudioConditionGraph::CenterNextItem(float totalWidth, float itemWidth)
{
	float pad = (totalWidth - itemWidth) * 0.5f;
	if (pad > 0.0f)
	{
		ImGui::Dummy(ImVec2(pad, 0.0f));
		ImGui::SameLine(0.0f, 0.0f);
	}
}
// 値の型ごとの表示色を返す
ImColor StudioConditionGraph::GetValueTypeColor(ValueType type)
{
	switch (type)
	{
	case ValueType::Int:   return ImColor(68, 201, 156, 255);  // 水色
	case ValueType::Float: return ImColor(147, 226, 74, 255);  // 緑
	case ValueType::Bool:  return ImColor(220, 48, 48, 255);   // 赤
	default:               return ImColor(255, 255, 255, 255);
	}
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
	const float nodeWidth = 140.0f;
	const float valueWidth = 80.0f;

	for (auto& v : targetLink->condition.variableNodes)
	{
		ed::NodeId nodeId(v.id);
		if (needsApplyPositions)
		{
			ed::SetNodePosition(nodeId, ImVec2(v.posX, v.posY));
		}

		// 型ごとに枠線の色を変える
		ed::PushStyleColor(ed::StyleColor_NodeBorder, GetValueTypeColor(v.type));
		ed::PushStyleVar(ed::StyleVar_NodeBorderWidth, 2.0f);


		ed::BeginNode(nodeId);
		ImGui::PushID(v.id);

		// 左に入力ピン、ノード名は中央寄せ、右に出力ピン
		ed::BeginPin(GetVariableInPinId(v.id), ed::PinKind::Input);
		ImGui::Text(u8"▶");
		ed::EndPin();

		const float pinIconWidth = ImGui::CalcTextSize(u8"▶").x;
		const float totalWidth = nodeWidth + pinIconWidth;

		float nameWidth = ImGui::CalcTextSize("Variable").x;
		float namePadding = (totalWidth - nameWidth) * 0.5f;

		ImGui::SameLine(namePadding > 0.0f ? namePadding : 0.0f);
		ImGui::Text("Variable");

		ImGui::SameLine(nodeWidth);
		ed::BeginPin(GetVariableOutPinId(v.id), ed::PinKind::Output);
		ImGui::Text(u8"▶");
		ed::EndPin();

		// int型ノード
		if (v.type == ValueType::Int)
		{
			CenterNextItem(totalWidth, valueWidth);
			ImGui::SetNextItemWidth(valueWidth);
			ImGui::DragInt("##value", &v.intValue, 1);
		}
		// float型ノード
		else if (v.type == ValueType::Float)
		{
			CenterNextItem(totalWidth, valueWidth);
			ImGui::SetNextItemWidth(valueWidth);
			ImGui::DragFloat("##value", &v.floatValue, 0.1f);
		}
		// bool型ノード
		else
		{
			float checkboxWidth = ImGui::GetFrameHeight();
			CenterNextItem(totalWidth, checkboxWidth);
			ImGui::Checkbox("##value", &v.boolValue);
		}

		ImGui::PopID();
		ed::EndNode();

		ed::PopStyleColor(1);
		ed::PopStyleVar(1);

		ImVec2 currentPos = ed::GetNodePosition(nodeId);
		v.posX = currentPos.x;
		v.posY = currentPos.y;
	}
}
// 比較演算ノード描画
void StudioConditionGraph::DrawOperatorNodes()
{
	const float nodeWidth = 100.0f;
	const ImColor operatorBorderColor(90, 140, 220, 255);

	for (auto& o : targetLink->condition.operatorNodes)
	{
		ed::NodeId nodeId(o.id);
		if (needsApplyPositions)
		{
			ed::SetNodePosition(nodeId, ImVec2(o.posX, o.posY));
		}

		ed::PushStyleColor(ed::StyleColor_NodeBorder, operatorBorderColor);
		ed::PushStyleVar(ed::StyleVar_NodeBorderWidth, 2.0f);

		ed::BeginNode(nodeId);
		ImGui::PushID(o.id);

		// ノード名(中央寄せ)
		float nameWidth = ImGui::CalcTextSize("Operator").x;
		CenterNextItem(nodeWidth, nameWidth);
		ImGui::Text("Operator");

		// A側入力ピン
		ed::BeginPin(GetOperatorPinA(o.id), ed::PinKind::Input);
		ImGui::Text(u8"▶");
		ed::EndPin();

		// 演算子記号(中央寄せ、少し大きめに表示)
		const char* symbol = GetCompareOpSymbol(o.op);
		const float symbolScale = 2.0f;

		ImGui::SetWindowFontScale(symbolScale);
		float symbolWidth = ImGui::CalcTextSize(symbol).x;
		CenterNextItem(nodeWidth, symbolWidth);
		ImGui::Text("%s", symbol);
		ImGui::SetWindowFontScale(1.0f);

		// B側入力ピンと、出力ピン(同じ行)
		ed::BeginPin(GetOperatorPinB(o.id), ed::PinKind::Input);
		ImGui::Text(u8"▶");
		ed::EndPin();

		ImGui::SameLine(nodeWidth);
		ed::BeginPin(GetOperatorOutPinId(o.id), ed::PinKind::Output);
		ImGui::Text(u8"▶");
		ed::EndPin();

		ImGui::PopID();
		ed::EndNode();

		ed::PopStyleColor(1);
		ed::PopStyleVar(1);

		ImVec2 currentPos = ed::GetNodePosition(nodeId);
		o.posX = currentPos.x;
		o.posY = currentPos.y;
	}
}

// ゲッターノード描画
void StudioConditionGraph::DrawGetterNodes()
{
	const float nodeWidth = 160.0f;

	for (auto& g : targetLink->condition.getterNodes)
	{
		ed::NodeId nodeId(g.id);
		if (needsApplyPositions)
		{
			ed::SetNodePosition(nodeId, ImVec2(g.posX, g.posY));
		}

		bool isAssigned = !g.componentName.empty() && !g.fieldName.empty();
		ImColor borderColor = isAssigned ? GetValueTypeColor(g.type) : ImColor(255, 255, 255, 255);

		ed::PushStyleColor(ed::StyleColor_NodeBorder, borderColor);
		ed::PushStyleVar(ed::StyleVar_NodeBorderWidth, 2.0f);

		ed::BeginNode(nodeId);
		ImGui::PushID(g.id);

		std::string label = isAssigned ? (g.componentName + "." + g.fieldName) : "(Unassigned)";
		float labelWidth = ImGui::CalcTextSize(label.c_str()).x;

		CenterNextItem(nodeWidth, labelWidth);
		ImGui::Text("%s", label.c_str());

		ImGui::SameLine(nodeWidth);
		ed::BeginPin(GetGetterOutPinId(g.id), ed::PinKind::Output);
		ImGui::Text(u8"▶");
		ed::EndPin();

		ImGui::PopID();
		ed::EndNode();

		ed::PopStyleColor(1);
		ed::PopStyleVar(1);

		ImVec2 currentPos = ed::GetNodePosition(nodeId);
		g.posX = currentPos.x;
		g.posY = currentPos.y;
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
		if (ImGui::BeginMenu("Add Variable"))
		{
			if (ImGui::MenuItem("int"))
			{
				CreateVariableNode(ValueType::Int);
			}
			if (ImGui::MenuItem("float"))
			{
				CreateVariableNode(ValueType::Float);
			}
			if (ImGui::MenuItem("bool"))
			{
				CreateVariableNode(ValueType::Bool);
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Add Operator"))
		{
			if (ImGui::MenuItem(">"))  CreateOperatorNode(CompareOp::Greater);
			if (ImGui::MenuItem("<"))  CreateOperatorNode(CompareOp::Less);
			if (ImGui::MenuItem("==")) CreateOperatorNode(CompareOp::Equal);
			if (ImGui::MenuItem("!=")) CreateOperatorNode(CompareOp::NotEqual);
			if (ImGui::MenuItem(">=")) CreateOperatorNode(CompareOp::GreaterEqual);
			if (ImGui::MenuItem("<=")) CreateOperatorNode(CompareOp::LessEqual);
			ImGui::EndMenu();
		}

		if (ImGui::MenuItem("Add Getter"))
		{
			CreateGetterNode();
		}

		ImGui::EndPopup();
	}

	ed::Resume();
}

// 変数ノード作成
void StudioConditionGraph::CreateVariableNode(ValueType type)
{
	ImVec2 canvasPos = ed::ScreenToCanvas(pendingCreatePos);

	ConditionVariableNode v;
	v.id = targetLink->condition.GenerateId();
	v.posX = canvasPos.x;
	v.posY = canvasPos.y;
	v.type = type;

	targetLink->condition.variableNodes.push_back(v);

	ed::SetNodePosition(ed::NodeId(v.id), canvasPos);
}
// 比較演算ノード作成
void StudioConditionGraph::CreateOperatorNode(CompareOp op)
{
	ImVec2 canvasPos = ed::ScreenToCanvas(pendingCreatePos);

	ConditionOperatorNode o;
	o.id = targetLink->condition.GenerateId();
	o.posX = canvasPos.x;
	o.posY = canvasPos.y;
	o.op = op;

	targetLink->condition.operatorNodes.push_back(o);

	ed::SetNodePosition(ed::NodeId(o.id), canvasPos);
}

// ゲッターノード作成
void StudioConditionGraph::CreateGetterNode()
{
	ImVec2 canvasPos = ed::ScreenToCanvas(pendingCreatePos);

	ConditionGetterNode g;
	g.id = targetLink->condition.GenerateId();
	g.posX = canvasPos.x;
	g.posY = canvasPos.y;

	targetLink->condition.getterNodes.push_back(g);

	ed::SetNodePosition(ed::NodeId(g.id), canvasPos);
}

// 比較演算子の表示用記号
const char* StudioConditionGraph::GetCompareOpSymbol(CompareOp op)
{
	switch (op)
	{
	case CompareOp::Greater:      return ">";
	case CompareOp::Less:         return "<";
	case CompareOp::Equal:        return "==";
	case CompareOp::NotEqual:     return "!=";
	case CompareOp::GreaterEqual: return ">=";
	case CompareOp::LessEqual:    return "<=";
	}
	return "?";
}

// ノード選択状態を更新
void StudioConditionGraph::UpdateSelection()
{
	selectedVariable = nullptr;
	selectedGetter = nullptr;
	selectedOperator = nullptr;

	if (ed::GetSelectedObjectCount() == 0) return;

	ed::NodeId selectedId;
	ed::GetSelectedNodes(&selectedId, 1);
	int id = (int)selectedId.Get();

	for (auto& v : targetLink->condition.variableNodes)
	{
		if (v.id == id) { selectedVariable = &v; return; }
	}
	for (auto& g : targetLink->condition.getterNodes)
	{
		if (g.id == id) { selectedGetter = &g; return; }
	}
	for (auto& o : targetLink->condition.operatorNodes)
	{
		if (o.id == id) { selectedOperator = &o; return; }
	}
}
