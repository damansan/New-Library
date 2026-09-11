#include "StudioConditionGraph.h"
#include <algorithm>
#include <vector>

#include "Tool/Editor/MetaRegistry.h"

void StudioConditionGraph::Initialize()
{
	canvas.Initialize();
}

void StudioConditionGraph::SetTarget(ConditionGraph* condition)
{
	targetCondition = condition;
	needsApplyPositions = true;
}

// ImGui描画
void StudioConditionGraph::DrawGUI()
{
	if (!targetCondition)
	{
		ImGui::TextDisabled("No link selected.");
		return;
	}

	canvas.Begin("ConditionCanvas");

	ed::PushStyleVar(ed::StyleVar_LinkStrength, 0.0f);

	DrawReturnNode();
	DrawVariableNodes();
	DrawOperatorNodes();
	DrawLogicNodes();
	DrawGetterNodes();

	ed::PopStyleVar(1);

	DrawConditionLinks();
	DrawLinkCreation();

	DrawCreateNodeMenu();

	UpdateSelection();

	canvas.End();

	needsApplyPositions = false;
}
// 右側メニュー
void StudioConditionGraph::DrawInspector()
{
	// 変数ノード選択
	if (selectedVariable)
	{
		ImGui::Text("Variable");
		ImGui::Separator();

		char nameBuf[128];
		strncpy_s(nameBuf, selectedVariable->name.c_str(), sizeof(nameBuf) - 1);
		if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf)))
		{
			selectedVariable->name = nameBuf;
		}

		ImGui::Text("Type: %s", GetValueTypeName(selectedVariable->type));

		switch (selectedVariable->type)
		{
		case ValueType::Int:
			ImGui::DragInt("Value", &selectedVariable->intValue, 1);
			break;
		case ValueType::Float:
			ImGui::DragFloat("Value", &selectedVariable->floatValue, 0.1f);
			break;
		case ValueType::Bool:
			ImGui::Checkbox("Value", &selectedVariable->boolValue);
			break;
		}

		DrawDeleteButton("Delete Node", selectedVariable->id, [this](int id) { DeleteVariableNode(id); });
		return;
	}

	// ゲッターノード選択
	if (selectedGetter)
	{
		ImGui::Text("Getter");
		ImGui::Separator();

		const auto& fields = MetaRegistry::GetAllFields();

		// 1段目:コンポーネント名の一覧(重複除去)
		std::vector<std::string> componentNames;
		for (const auto& f : fields)
		{
			if (std::find(componentNames.begin(), componentNames.end(), f.componentName) == componentNames.end())
			{
				componentNames.push_back(f.componentName);
			}
		}

		std::string currentComponent = selectedGetter->componentName.empty() ? "(Select Component)" : selectedGetter->componentName;

		if (ImGui::BeginCombo("Component", currentComponent.c_str()))
		{
			for (const auto& name : componentNames)
			{
				bool isSelected = (name == selectedGetter->componentName);
				if (ImGui::Selectable(name.c_str(), isSelected))
				{
					// コンポーネントを変えたら、フィールドの選択はリセットする
					selectedGetter->componentName = name;
					selectedGetter->fieldName.clear();
				}
			}
			ImGui::EndCombo();
		}

		// 2段目:選択中コンポーネントに属するフィールドだけに絞り込む
		if (!selectedGetter->componentName.empty())
		{
			std::string currentField = selectedGetter->fieldName.empty() ? "(Select Field)" : selectedGetter->fieldName;

			if (ImGui::BeginCombo("Field", currentField.c_str()))
			{
				for (const auto& f : fields)
				{
					if (f.componentName != selectedGetter->componentName) continue;

					bool isSelected = (f.fieldLabel == selectedGetter->fieldName);
					if (ImGui::Selectable(f.fieldLabel.c_str(), isSelected))
					{
						// フィールドを変えたら、成分選択もリセットする
						selectedGetter->fieldName = f.fieldLabel;
						selectedGetter->type = f.type;
						selectedGetter->selectedComponent = FloatComponent::None;
					}
				}
				ImGui::EndCombo();
			}
		}

		// 3段目:選択中フィールドがFloat2/3/4の場合のみ、成分選択を表示する
		int componentCount = MetaRegistry::GetComponentCount(selectedGetter->type);
		if (componentCount > 0)
		{
			static const char* names[] = { "X", "Y", "Z", "W" };
			static const FloatComponent values[] = { FloatComponent::X, FloatComponent::Y, FloatComponent::Z, FloatComponent::W };

			std::string currentComp = "(Select Component)";
			for (int i = 0; i < componentCount; i++)
			{
				if (values[i] == selectedGetter->selectedComponent) currentComp = names[i];
			}

			if (ImGui::BeginCombo("Axis", currentComp.c_str()))
			{
				for (int i = 0; i < componentCount; i++)
				{
					bool isSelected = (values[i] == selectedGetter->selectedComponent);
					if (ImGui::Selectable(names[i], isSelected))
					{
						selectedGetter->selectedComponent = values[i];
					}
				}
				ImGui::EndCombo();
			}
		}

		DrawDeleteButton("Delete Getter", selectedGetter->id, [this](int id) { DeleteGetterNode(id); });
		return;
	}

	// 比較演算ノード選択
	if (selectedOperator)
	{
		ImGui::Text("Operator");
		ImGui::Separator();

		ImGui::Text("Op: %s", GetCompareOpSymbol(selectedOperator->op));

		if (ImGui::Button("Invert", ImVec2(-1.0f, 0.0f)))
		{
			selectedOperator->op = InvertCompareOp(selectedOperator->op);
		}

		DrawDeleteButton("Delete Operator", selectedOperator->id, [this](int id) { DeleteOperatorNode(id); });
		return;
	}

	// 論理演算ノード選択
	if (selectedLogic)
	{
		ImGui::Text("Logic");
		ImGui::Separator();

		ImGui::Text("Op: %s", GetLogicOpSymbol(selectedLogic->op));

		if (ImGui::Button("Toggle AND/OR", ImVec2(-1.0f, 0.0f)))
		{
			selectedLogic->op = (selectedLogic->op == LogicOp::And) ? LogicOp::Or : LogicOp::And;
		}

		DrawDeleteButton("Delete Logic", selectedLogic->id, [this](int id) { DeleteLogicNode(id); });
		return;
	}

	// リンク選択
	if (selectedConditionLink)
	{
		ImGui::Text("Link");
		ImGui::Separator();

		int linkId = selectedConditionLink->id;
		DrawDeleteButton("Delete Link", linkId, [this](int id) { DeleteConditionLink(id); });
		return;
	}

	ImGui::TextDisabled("No node selected.");
}


// リターンノード描画
void StudioConditionGraph::DrawReturnNode()
{
	ConditionReturnNode& returnNode = targetCondition->returnNode;
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

	ed::BeginPin(GetReturnInPinId(returnNode.id), ed::PinKind::Input);
	ImGui::Text(u8"▶");
	ed::EndPin();

	ImGui::SameLine();

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

	for (auto& v : targetCondition->variableNodes) 
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

		std::string displayName = v.name.empty() ? "Variable" : v.name;
		float nameWidth = ImGui::CalcTextSize(displayName.c_str()).x;
		float namePadding = (totalWidth - nameWidth) * 0.5f;

		ImGui::SameLine(namePadding > 0.0f ? namePadding : 0.0f);
		ImGui::Text("%s", displayName.c_str());

		ImGui::SameLine(nodeWidth);
		ed::BeginPin(GetVariableOutPinId(v.id), ed::PinKind::Output);
		ImGui::Text(u8"▶");
		ed::EndPin();

		// int型ノード
		if (v.type == ValueType::Int)
		{
			CenterNextItem(totalWidth, valueWidth);
			ImGui::SetNextItemWidth(valueWidth);
			ImGui::DragInt("##int", &v.intValue, 1);
		}
		// float型ノード
		else if (v.type == ValueType::Float)
		{
			CenterNextItem(totalWidth, valueWidth);
			ImGui::SetNextItemWidth(valueWidth);
			ImGui::DragFloat("##float", &v.floatValue, 0.1f);
		}
		// bool型ノード
		else
		{
			float checkboxWidth = ImGui::GetFrameHeight();
			CenterNextItem(totalWidth, checkboxWidth);
			ImGui::Checkbox("##bool", &v.boolValue);
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

	for (auto& o : targetCondition->operatorNodes)
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
// 論理演算ノード描画
void StudioConditionGraph::DrawLogicNodes()
{
	const float nodeWidth = 100.0f;
	const ImColor logicBorderColor(200, 130, 60, 255); // オレンジ

	for (auto& l : targetCondition->logicNodes)
	{
		ed::NodeId nodeId(l.id);
		if (needsApplyPositions)
		{
			ed::SetNodePosition(nodeId, ImVec2(l.posX, l.posY));
		}

		ed::PushStyleColor(ed::StyleColor_NodeBorder, logicBorderColor);
		ed::PushStyleVar(ed::StyleVar_NodeBorderWidth, 2.0f);

		ed::BeginNode(nodeId);
		ImGui::PushID(l.id);

		// A側入力ピン
		ed::BeginPin(GetLogicPinA(l.id), ed::PinKind::Input);
		ImGui::Text(u8"▶");
		ed::EndPin();

		// 演算子記号(中央寄せ、少し大きめに表示)
		const char* symbol = GetLogicOpSymbol(l.op);
		const float symbolScale = 1.5f;

		ImGui::SetWindowFontScale(symbolScale);
		float symbolWidth = ImGui::CalcTextSize(symbol).x;
		CenterNextItem(nodeWidth, symbolWidth);
		ImGui::Text("%s", symbol);
		ImGui::SetWindowFontScale(1.0f);

		// B側入力ピンと、出力ピン(同じ行)
		ed::BeginPin(GetLogicPinB(l.id), ed::PinKind::Input);
		ImGui::Text(u8"▶");
		ed::EndPin();

		ImGui::SameLine(nodeWidth);
		ed::BeginPin(GetLogicOutPinId(l.id), ed::PinKind::Output);
		ImGui::Text(u8"▶");
		ed::EndPin();

		ImGui::PopID();
		ed::EndNode();

		ed::PopStyleColor(1);
		ed::PopStyleVar(1);

		ImVec2 currentPos = ed::GetNodePosition(nodeId);
		l.posX = currentPos.x;
		l.posY = currentPos.y;
	}
}
// ゲッターノード描画
void StudioConditionGraph::DrawGetterNodes()
{
	for (auto& g : targetCondition->getterNodes)
	{
		ed::NodeId nodeId(g.id);
		if (needsApplyPositions)
		{
			ed::SetNodePosition(nodeId, ImVec2(g.posX, g.posY));
		}

		bool isAssigned = !g.componentName.empty() && !g.fieldName.empty();

		if (isAssigned)
		{
			DrawAssignedGetterNode(g);
		}
		else
		{
			DrawUnassignedGetterNode(g);
		}

		ImVec2 currentPos = ed::GetNodePosition(nodeId);
		g.posX = currentPos.x;
		g.posY = currentPos.y;
	}
}

// 未設定ゲッターノード
void StudioConditionGraph::DrawUnassignedGetterNode(ConditionGetterNode& g)
{
	ed::NodeId nodeId(g.id);

	ed::PushStyleColor(ed::StyleColor_NodeBorder, ImColor(255, 255, 255, 255));
	ed::PushStyleVar(ed::StyleVar_NodeBorderWidth, 2.0f);

	ed::BeginNode(nodeId);
	ImGui::PushID(g.id);

	const char* label = "Empty";
	const float pinIconWidth = ImGui::CalcTextSize(u8"▶").x;
	const float labelWidth = ImGui::CalcTextSize(label).x;
	const float spacing = 8.0f;

	const float nodeWidth = labelWidth + spacing;

	ImGui::Text("%s", label);

	ImGui::SameLine(nodeWidth);
	ed::BeginPin(GetGetterOutPinId(g.id), ed::PinKind::Output);
	ImGui::Text(u8"▶");
	ed::EndPin();

	ImGui::PopID();
	ed::EndNode();

	ed::PopStyleColor(1);
	ed::PopStyleVar(1);
}
// 設定済みゲッターノード
void StudioConditionGraph::DrawAssignedGetterNode(ConditionGetterNode& g)
{
	ed::NodeId nodeId(g.id);

	ed::PushStyleColor(ed::StyleColor_NodeBorder, GetValueTypeColor(g.type));
	ed::PushStyleVar(ed::StyleVar_NodeBorderWidth, 2.0f);

	ed::BeginNode(nodeId);
	ImGui::PushID(g.id);

	// 1行目:コンポーネント名
	std::string line1 = g.componentName;

	// 2行目:フィールド名(構造体の場合は「フィールド名.成分」、単純な型ならそのまま)
	std::string line2;
	int componentCount = MetaRegistry::GetComponentCount(g.type);
	if (componentCount > 0)
	{
		std::string axisName = "?"; // 未選択の場合
		switch (g.selectedComponent)
		{
		case FloatComponent::X: axisName = "X"; break;
		case FloatComponent::Y: axisName = "Y"; break;
		case FloatComponent::Z: axisName = "Z"; break;
		case FloatComponent::W: axisName = "W"; break;
		default: break;
		}
		line2 = g.fieldName + "." + axisName;
	}
	else
	{
		line2 = g.fieldName;
	}

	float line1Width = ImGui::CalcTextSize(line1.c_str()).x;
	float line2Width = ImGui::CalcTextSize(line2.c_str()).x;
	const float pinIconWidth = ImGui::CalcTextSize(u8"▶").x;
	const float spacing = 8.0f;

	// 2行のうち長い方に合わせて、ノード幅を決める
	const float contentWidth = (line1Width > line2Width) ? line1Width : line2Width;
	const float nodeWidth = contentWidth + spacing;

	CenterNextItem(nodeWidth, line1Width);
	ImGui::Text("%s", line1.c_str());

	CenterNextItem(nodeWidth, line2Width);
	ImGui::Text("%s", line2.c_str());

	ImGui::SameLine(nodeWidth);
	ed::BeginPin(GetGetterOutPinId(g.id), ed::PinKind::Output);
	ImGui::Text(u8"▶");
	ed::EndPin();

	ImGui::PopID();
	ed::EndNode();

	ed::PopStyleColor(1);
	ed::PopStyleVar(1);
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
		// 変数
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

		// 比較演算子
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

		// 論理演算子
		if (ImGui::BeginMenu("Add Logic"))
		{
			if (ImGui::MenuItem("AND")) CreateLogicNode(LogicOp::And);
			if (ImGui::MenuItem("OR"))  CreateLogicNode(LogicOp::Or);
			ImGui::EndMenu();
		}

		// コンポーネントのゲッター
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
	v.id = targetCondition->GenerateId();
	v.posX = canvasPos.x;
	v.posY = canvasPos.y;
	v.type = type;

	targetCondition->variableNodes.push_back(v);

	ed::SetNodePosition(ed::NodeId(v.id), canvasPos);
}
// 比較演算ノード作成
void StudioConditionGraph::CreateOperatorNode(CompareOp op)
{
	ImVec2 canvasPos = ed::ScreenToCanvas(pendingCreatePos);

	ConditionOperatorNode o;
	o.id = targetCondition->GenerateId();
	o.posX = canvasPos.x;
	o.posY = canvasPos.y;
	o.op = op;

	targetCondition->operatorNodes.push_back(o);

	ed::SetNodePosition(ed::NodeId(o.id), canvasPos);
}
// 論理演算ノード作成
void StudioConditionGraph::CreateLogicNode(LogicOp op)
{
	ImVec2 canvasPos = ed::ScreenToCanvas(pendingCreatePos);

	ConditionLogicNode l;
	l.id = targetCondition->GenerateId();
	l.posX = canvasPos.x;
	l.posY = canvasPos.y;
	l.op = op;

	targetCondition->logicNodes.push_back(l);

	ed::SetNodePosition(ed::NodeId(l.id), canvasPos);
}
// ゲッターノード作成
void StudioConditionGraph::CreateGetterNode()
{
	ImVec2 canvasPos = ed::ScreenToCanvas(pendingCreatePos);

	ConditionGetterNode g;
	g.id = targetCondition->GenerateId();
	g.posX = canvasPos.x;
	g.posY = canvasPos.y;

	targetCondition->getterNodes.push_back(g);

	ed::SetNodePosition(ed::NodeId(g.id), canvasPos);
}


// 削除ボタン
void StudioConditionGraph::DrawDeleteButton(const char* label, int targetId, const std::function<void(int)>& onDelete)
{
	ImGui::Spacing();
	ImGui::Spacing();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.15f, 0.15f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.2f, 0.2f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));

	bool pressed = ImGui::Button(label, ImVec2(-1.0f, 0.0f));

	ImGui::PopStyleColor(3);

	if (pressed)
	{
		onDelete(targetId);
		selectedVariable = nullptr;
		selectedGetter = nullptr;
		selectedOperator = nullptr;
		selectedLogic = nullptr;
		selectedConditionLink = nullptr;
	}
}
// 変数ノード削除
void StudioConditionGraph::DeleteVariableNode(int id)
{
	auto& vars = targetCondition->variableNodes;
	vars.erase(
		std::remove_if(vars.begin(), vars.end(),
			[id](const ConditionVariableNode& v) { return v.id == id; }),
		vars.end()
	);

	auto& links = targetCondition->links;
	links.erase(
		std::remove_if(links.begin(), links.end(),
			[id](const ConditionLink& l) { return l.fromId == id || l.toId == id; }),
		links.end()
	);
}
// ゲッターノード削除
void StudioConditionGraph::DeleteGetterNode(int id)
{
	auto& getters = targetCondition->getterNodes;
	getters.erase(
		std::remove_if(getters.begin(), getters.end(),
			[id](const ConditionGetterNode& g) { return g.id == id; }),
		getters.end()
	);

	auto& links = targetCondition->links;
	links.erase(
		std::remove_if(links.begin(), links.end(),
			[id](const ConditionLink& l) { return l.fromId == id || l.toId == id; }),
		links.end()
	);
}
// 比較演算ノード削除
void StudioConditionGraph::DeleteOperatorNode(int id)
{
	auto& ops = targetCondition->operatorNodes;
	ops.erase(
		std::remove_if(ops.begin(), ops.end(),
			[id](const ConditionOperatorNode& o) { return o.id == id; }),
		ops.end()
	);

	auto& links = targetCondition->links;
	links.erase(
		std::remove_if(links.begin(), links.end(),
			[id](const ConditionLink& l) { return l.fromId == id || l.toId == id; }),
		links.end()
	);
}
// 論理演算ノード削除
void StudioConditionGraph::DeleteLogicNode(int id)
{
	auto& logics = targetCondition->logicNodes;
	logics.erase(
		std::remove_if(logics.begin(), logics.end(),
			[id](const ConditionLogicNode& l) { return l.id == id; }),
		logics.end()
	);

	auto& links = targetCondition->links;
	links.erase(
		std::remove_if(links.begin(), links.end(),
			[id](const ConditionLink& cl) { return cl.fromId == id || cl.toId == id; }),
		links.end()
	);
}
// ConditionLink削除
void StudioConditionGraph::DeleteConditionLink(int id)
{
	auto& links = targetCondition->links;
	links.erase(
		std::remove_if(links.begin(), links.end(),
			[id](const ConditionLink& l) { return l.id == id; }),
		links.end()
	);
}


// ノード選択状態を更新
void StudioConditionGraph::UpdateSelection()
{
	selectedVariable = nullptr;
	selectedGetter = nullptr;
	selectedOperator = nullptr;
	selectedLogic = nullptr;
	selectedConditionLink = nullptr;

	if (ed::GetSelectedObjectCount() == 0) return;

	ed::NodeId selectedId;
	ed::GetSelectedNodes(&selectedId, 1);
	int id = (int)selectedId.Get();

	for (auto& v : targetCondition->variableNodes)
	{
		if (v.id == id) { selectedVariable = &v; return; }
	}
	for (auto& g : targetCondition->getterNodes)
	{
		if (g.id == id) { selectedGetter = &g; return; }
	}
	for (auto& o : targetCondition->operatorNodes)
	{
		if (o.id == id) { selectedOperator = &o; return; }
	}
	for (auto& l : targetCondition->logicNodes)
	{
		if (l.id == id) { selectedLogic = &l; return; }
	}

	// Linkの選択
	selectedConditionLink = GetSelectedConditionLink();
}
// 選択中のConditionLinkを取得する
ConditionLink* StudioConditionGraph::GetSelectedConditionLink()
{
	ed::LinkId selectedId;
	if (ed::GetSelectedLinks(&selectedId, 1) == 0) return nullptr;

	for (auto& cl : targetCondition->links)
	{
		if (cl.id == (int)selectedId.Get()) return &cl;
	}
	return nullptr;
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
	case ValueType::Int:    return ImColor(68, 201, 156, 255);  // 水色

	case ValueType::Float:  return ImColor(147, 226, 74, 255);  // 緑(Float2/3/4も同じ緑にする)
	case ValueType::Float2:
	case ValueType::Float3:
	case ValueType::Float4: return ImColor(147, 226, 74, 255);

	case ValueType::Bool:   return ImColor(220, 48, 48, 255);   // 赤

	default:                return ImColor(255, 255, 255, 255);
	}
}
// 値の型名を文字にして返す
const char* StudioConditionGraph::GetValueTypeName(ValueType type)
{
	switch (type)
	{
	case ValueType::Int:   return "int";
	case ValueType::Float: return "float";
	case ValueType::Bool:  return "bool";
	}
	return "?";
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
// 比較演算子の向きを反転する
CompareOp StudioConditionGraph::InvertCompareOp(CompareOp op)
{
	switch (op)
	{
	case CompareOp::Greater:      return CompareOp::Less;
	case CompareOp::Less:         return CompareOp::Greater;
	case CompareOp::GreaterEqual: return CompareOp::LessEqual;
	case CompareOp::LessEqual:    return CompareOp::GreaterEqual;
	default: return op; // Equal/NotEqualは向きの概念が無いため変化なし
	}
}

// 論理演算子の表示用記号
const char* StudioConditionGraph::GetLogicOpSymbol(LogicOp op)
{
	switch (op)
	{
	case LogicOp::And: return "AND";
	case LogicOp::Or:  return "OR";
	}
	return "?";
}


// 生のピンID(int64_t)から、役割とノードIDを逆算する
void StudioConditionGraph::ResolvePinRole(int64_t rawPinId, PinRole& outRole, int& outNodeId)
{
	if (rawPinId >= ReturnOffset && rawPinId < ReturnOffset + 1000000)
	{
		outRole = PinRole::ReturnIn;
		outNodeId = (int)(rawPinId - ReturnOffset);
		return;
	}
	if (rawPinId >= GetterOffset && rawPinId < GetterOffset + 1000000)
	{
		outRole = PinRole::GetterOut;
		outNodeId = (int)(rawPinId - GetterOffset);
		return;
	}
	if (rawPinId >= LogicOffset && rawPinId < LogicOffset + 1000000)
	{
		int64_t offset = rawPinId - LogicOffset;
		int64_t nodeId = offset / 3;
		int64_t rem = offset % 3;
		outNodeId = (int)nodeId;
		outRole = (rem == 0) ? PinRole::LogicA : (rem == 1) ? PinRole::LogicB : PinRole::LogicOut;
		return;
	}
	if (rawPinId >= OperatorOffset && rawPinId < OperatorOffset + 1000000)
	{
		int64_t offset = rawPinId - OperatorOffset;
		int64_t nodeId = offset / 3;
		int64_t rem = offset % 3;
		outNodeId = (int)nodeId;
		outRole = (rem == 0) ? PinRole::OperatorA : (rem == 1) ? PinRole::OperatorB : PinRole::OperatorOut;
		return;
	}
	if (rawPinId >= VariableOffset && rawPinId < VariableOffset + 1000000)
	{
		int64_t offset = rawPinId - VariableOffset;
		int64_t nodeId = offset / 2;
		int64_t rem = offset % 2;
		outNodeId = (int)nodeId;
		outRole = (rem == 0) ? PinRole::VariableIn : PinRole::VariableOut;
		return;
	}

	outRole = PinRole::Unknown;
	outNodeId = -1;
}
// 生のノードID(int)から、このLink内でどの種類のノードかを調べる
StudioConditionGraph::NodeKind StudioConditionGraph::GetNodeKind(int id) const
{
	if (targetCondition->returnNode.id == id) return NodeKind::Return;

	for (const auto& v : targetCondition->variableNodes)
		if (v.id == id) return NodeKind::Variable;

	for (const auto& g : targetCondition->getterNodes)
		if (g.id == id) return NodeKind::Getter;

	for (const auto& o : targetCondition->operatorNodes)
		if (o.id == id) return NodeKind::Operator;

	for (const auto& l : targetCondition->logicNodes)
		if (l.id == id) return NodeKind::Logic;

	return NodeKind::Unknown;
}
// ピン接続(ドラッグ)の処理
void StudioConditionGraph::DrawLinkCreation()
{
	if (ed::BeginCreate())
	{
		ed::PinId pinIdA, pinIdB;
		if (ed::QueryNewLink(&pinIdA, &pinIdB))
		{
			PinRole roleA, roleB;
			int nodeIdA, nodeIdB;
			ResolvePinRole(pinIdA.Get(), roleA, nodeIdA);
			ResolvePinRole(pinIdB.Get(), roleB, nodeIdB);

			// 出力側/入力側を判定する(ドラッグの向きに関わらず正規化する)
			auto isOutputRole = [](PinRole r)
				{
					return r == PinRole::VariableOut || r == PinRole::GetterOut || r == PinRole::OperatorOut || r == PinRole::LogicOut;
				};
			auto isInputRole = [](PinRole r)
				{
					return r == PinRole::VariableIn || r == PinRole::OperatorA || r == PinRole::OperatorB
						|| r == PinRole::LogicA || r == PinRole::LogicB || r == PinRole::ReturnIn;
				};

			PinRole outRole, inRole;
			int outNodeId, inNodeId;
			bool orderOk = false;

			if (isOutputRole(roleA) && isInputRole(roleB))
			{
				outRole = roleA; outNodeId = nodeIdA;
				inRole = roleB; inNodeId = nodeIdB;
				orderOk = true;
			}
			else if (isOutputRole(roleB) && isInputRole(roleA))
			{
				outRole = roleB; outNodeId = nodeIdB;
				inRole = roleA; inNodeId = nodeIdA;
				orderOk = true;
			}

			// 出力側が「常にBoolな出力(Operator/Logic)」かどうか
			auto isBoolOutRole = [](PinRole r)
				{
					return r == PinRole::OperatorOut || r == PinRole::LogicOut;
				};

			// 許可する組み合わせ:
			// Getter/Variable/Operator/Logic → OperatorA/B
			// Getter/Variable(bool)/Operator/Logic → LogicA/B
			// Operator/Logic → Return
			// Getter/Variable(bool) → Return
			bool isValid = orderOk && (
				((outRole == PinRole::GetterOut || outRole == PinRole::VariableOut || isBoolOutRole(outRole)) && (inRole == PinRole::OperatorA || inRole == PinRole::OperatorB)) ||
				((outRole == PinRole::GetterOut || outRole == PinRole::VariableOut || isBoolOutRole(outRole)) && (inRole == PinRole::LogicA || inRole == PinRole::LogicB)) ||
				(isBoolOutRole(outRole) && inRole == PinRole::ReturnIn) ||
				((outRole == PinRole::GetterOut || outRole == PinRole::VariableOut) && inRole == PinRole::ReturnIn)
				);

			// 同じノード同士は禁止
			if (isValid && outNodeId == inNodeId) isValid = false;

			// Getter/Variable/Operator/Logic→OperatorのA/Bは、大小系演算子ならBoolを禁止する。==/!=なら型一致のみ見る
			if (isValid && (outRole == PinRole::GetterOut || outRole == PinRole::VariableOut || isBoolOutRole(outRole)) && (inRole == PinRole::OperatorA || inRole == PinRole::OperatorB))
			{
				ValueType srcType = GetOutputValueType(outRole, outNodeId);

				CompareOp op = CompareOp::Greater;
				for (const auto& o : targetCondition->operatorNodes)
				{
					if (o.id == inNodeId) { op = o.op; break; }
				}

				if (srcType == ValueType::Bool && IsOrderCompareOp(op))
				{
					isValid = false;
				}

				// 既にA/Bのどちらかに繋がっている側があれば、その型と一致している必要がある
				int otherPinNodeId = -1;
				PinRole otherOutRole = PinRole::Unknown;
				bool wantPinB = (inRole == PinRole::OperatorB);
				for (const auto& cl : targetCondition->links)
				{
					if (cl.toId == inNodeId && cl.toPinB != wantPinB)
					{
						otherPinNodeId = cl.fromId;
						NodeKind otherKind = GetNodeKind(cl.fromId);
						otherOutRole = (otherKind == NodeKind::Operator) ? PinRole::OperatorOut
							: (otherKind == NodeKind::Logic) ? PinRole::LogicOut
							: (otherKind == NodeKind::Getter) ? PinRole::GetterOut
							: PinRole::VariableOut;
						break;
					}
				}
				if (otherPinNodeId != -1)
				{
					ValueType otherType = GetOutputValueType(otherOutRole, otherPinNodeId);
					if (otherType != srcType) isValid = false;
				}
			}

			// Getter/Variable(bool)/Operator/Logic → LogicのA/Bは、Bool型のみ許可
			if (isValid && (outRole == PinRole::GetterOut || outRole == PinRole::VariableOut || isBoolOutRole(outRole)) && (inRole == PinRole::LogicA || inRole == PinRole::LogicB))
			{
				ValueType srcType = GetOutputValueType(outRole, outNodeId);
				if (srcType != ValueType::Bool) isValid = false;
			}

			// Getter/Variable(bool)→Returnは、bool型のみ許可
			if (isValid && (outRole == PinRole::GetterOut || outRole == PinRole::VariableOut) && inRole == PinRole::ReturnIn)
			{
				NodeKind outKind = (outRole == PinRole::GetterOut) ? NodeKind::Getter : NodeKind::Variable;
				ValueType outType = GetNodeValueType(outKind, outNodeId);
				if (outType != ValueType::Bool) isValid = false;
			}

			if (isValid)
			{
				if (ed::AcceptNewItem())
				{
					bool toPinB = (inRole == PinRole::OperatorB || inRole == PinRole::LogicB);

					// 入力ピンは1本まで:既に同じ入力に繋がっているLinkがあれば置き換える
					auto& links = targetCondition->links;
					links.erase(
						std::remove_if(links.begin(), links.end(),
							[&](const ConditionLink& l)
							{
								return l.toId == inNodeId && l.toPinB == toPinB;
							}),
						links.end()
					);

					ConditionLink newLink;
					newLink.id = targetCondition->GenerateId();
					newLink.fromId = outNodeId;
					newLink.toId = inNodeId;
					newLink.toPinB = toPinB;
					links.push_back(newLink);
				}
			}
			else
			{
				ed::RejectNewItem(ImColor(255, 0, 0, 255), 2.0f);
			}
		}
	}
	ed::EndCreate();
}
// 接続済みConditionLinkの描画
void StudioConditionGraph::DrawConditionLinks()
{
	const ImColor linkColor(210, 210, 210, 255);
	const float linkThickness = 2.0f;

	for (const auto& cl : targetCondition->links)
	{
		NodeKind fromKind = GetNodeKind(cl.fromId);
		NodeKind toKind = GetNodeKind(cl.toId);

		ed::PinId fromPin = GetOutputPinIdOf(cl.fromId, fromKind);
		ed::PinId toPin = GetInputPinIdOf(cl.toId, toKind, cl.toPinB);

		ed::Link(ed::LinkId(cl.id), fromPin, toPin, linkColor, linkThickness);
	}
}
// fromId/toIdからed::Linkを引くための、実際のピンIDを求める
ed::PinId StudioConditionGraph::GetOutputPinIdOf(int nodeId, NodeKind kind) const
{
	switch (kind)
	{
	case NodeKind::Variable: return GetVariableOutPinId(nodeId);
	case NodeKind::Getter:   return GetGetterOutPinId(nodeId);
	case NodeKind::Operator: return GetOperatorOutPinId(nodeId);
	case NodeKind::Logic:    return GetLogicOutPinId(nodeId);
	default:                 return ed::PinId();
	}
}
ed::PinId StudioConditionGraph::GetInputPinIdOf(int nodeId, NodeKind kind, bool toPinB) const
{
	switch (kind)
	{
	case NodeKind::Variable: return GetVariableInPinId(nodeId);
	case NodeKind::Operator: return toPinB ? GetOperatorPinB(nodeId) : GetOperatorPinA(nodeId);
	case NodeKind::Logic:    return toPinB ? GetLogicPinB(nodeId) : GetLogicPinA(nodeId);
	case NodeKind::Return:   return GetReturnInPinId(nodeId);
	default:                 return ed::PinId();
	}
}
// ノードの値の型を取得する
ValueType StudioConditionGraph::GetNodeValueType(NodeKind kind, int nodeId) const
{
	if (kind == NodeKind::Variable)
	{
		for (const auto& v : targetCondition->variableNodes)
			if (v.id == nodeId) return v.type;
	}
	else if (kind == NodeKind::Getter)
	{
		for (const auto& g : targetCondition->getterNodes)
		{
			if (g.id == nodeId)
			{
				// Float2/3/4は、軸(X/Y/Z/W)を選んで初めて「実質Float」として扱う
				if (MetaRegistry::GetComponentCount(g.type) > 0)
				{
					return (g.selectedComponent != FloatComponent::None) ? ValueType::Float : g.type;
				}
				return g.type;
			}
		}
	}
	return ValueType::Float;
}
// 大小系演算子か判定する
bool StudioConditionGraph::IsOrderCompareOp(CompareOp op)
{
	switch (op)
	{
	case CompareOp::Greater:
	case CompareOp::Less:
	case CompareOp::GreaterEqual:
	case CompareOp::LessEqual:
		return true;
	default:
		return false;
	}
}
// 出力ピンの実質的な型を取得する
ValueType StudioConditionGraph::GetOutputValueType(PinRole outRole, int outNodeId) const
{
	if (outRole == PinRole::OperatorOut) return ValueType::Bool; // Operatorの出力は常にBool
	if (outRole == PinRole::LogicOut)    return ValueType::Bool; // Logicの出力も常にBool
	if (outRole == PinRole::GetterOut)   return GetNodeValueType(NodeKind::Getter, outNodeId);
	return GetNodeValueType(NodeKind::Variable, outNodeId); // VariableOut
}