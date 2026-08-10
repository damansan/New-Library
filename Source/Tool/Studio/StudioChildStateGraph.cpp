#include "StudioChildStateGraph.h"

void StudioChildStateGraph::Initialize()
{
	canvas.Initialize();
}

// ImGui描画
void StudioChildStateGraph::DrawGUI(ChildStateGraph& childGraph, const std::vector<std::string>& animations)
{
	graph = &childGraph;
	availableAnimations = &animations;

	canvas.Begin("ChildStateCanvas");

	DrawStates();
	DrawLinks();

	DrawCreateStateMenu();
	DrawStateContextMenu();
	UpdateLinkCreation();
	DrawPendingLink();

	UpdateSelection();

	canvas.End();
}

// 右側メニュー
void StudioChildStateGraph::DrawInspector()
{
	if (selectedState)
	{
		DrawStateInspector(selectedState);
		return;
	}

	if (selectedLink)
	{
		DrawLinkInspector(selectedLink);
		return;
	}

	// どちらも選択されていない場合
	ImGui::TextDisabled("No state selected.");
}

// 画面遷移が要求されていれば取り出す
NavigationRequest StudioChildStateGraph::ConsumeNavigationRequest(AnimLink*& outLink)
{
	NavigationRequest request = pendingNavigation;
	outLink = (request == NavigationRequest::EditCondition) ? selectedLink : nullptr;

	pendingNavigation = NavigationRequest::None;
	return request;
}

// State描画
void StudioChildStateGraph::DrawStates()
{
	stateScreenRects.clear();

	for (auto& state : graph->children)
	{
		ed::NodeId nodeId(state.id);

		if (needsApplyPositions)
		{
			ed::SetNodePosition(nodeId, ImVec2(state.posX, state.posY));
		}

		PushStateStyle(state);

		ed::BeginNode(nodeId);

		float nodeWidth = (state.type != ChildStateType::Normal) ? 100.0f : ImGui::CalcTextSize("New State").x;
		float textWidth = ImGui::CalcTextSize(state.name.c_str()).x;

		CenterNextItem(nodeWidth, textWidth);
		ImGui::Text("%s", state.name.c_str());

		ImGui::SameLine(nodeWidth);
		ImGui::Dummy(ImVec2(0.0f, 0.0f));

		ed::EndNode();

		stateScreenRects[state.id] = { ImGui::GetItemRectMin(), ImGui::GetItemRectMax() };

		PopStateStyle(state);

		ImVec2 currentPos = ed::GetNodePosition(nodeId);
		state.posX = currentPos.x;
		state.posY = currentPos.y;
	}
	needsApplyPositions = false;
}
// Stateの種類に応じた見た目のスタイルを適用する
void StudioChildStateGraph::PushStateStyle(const ChildState& state)
{
	switch (state.type)
	{
	case ChildStateType::Start:
		ed::PushStyleColor(ed::StyleColor_NodeBg, ImColor(46, 125, 50, 255));
		ed::PushStyleColor(ed::StyleColor_NodeBorder, ImColor(27, 94, 32, 255));
		ed::PushStyleVar(ed::StyleVar_NodeRounding, 4.0f);
		break;

	case ChildStateType::End:
		ed::PushStyleColor(ed::StyleColor_NodeBg, ImColor(197, 134, 192, 255));
		ed::PushStyleColor(ed::StyleColor_NodeBorder, ImColor(140, 90, 137, 255));
		ed::PushStyleVar(ed::StyleVar_NodeRounding, 4.0f);
		break;

	case ChildStateType::Normal:
		ed::PushStyleColor(ed::StyleColor_NodeBg, ImColor(70, 90, 110, 255));
		ed::PushStyleColor(ed::StyleColor_NodeBorder, ImColor(42, 54, 66, 255));
		ed::PushStyleVar(ed::StyleVar_NodeRounding, 4.0f);
		break;
	}
}
// PushStateStyleで適用したスタイルを解除する
void StudioChildStateGraph::PopStateStyle(const ChildState& state)
{
	// どの種類でも、積んだ数(スタイル1つ+色2つ)は共通なので分岐不要
	ed::PopStyleVar(1);
	ed::PopStyleColor(2);
}


// Link描画
void StudioChildStateGraph::DrawLinks()
{
	const float arrowTrim = 10.0f; // 矢印の底辺の位置まで、線を手前に引く量

	for (const auto& link : graph->childLinks)
	{
		ImVec2 startPoint, endPoint;
		CalcLinkPoints(link, startPoint, endPoint);

		if (startPoint.x == 0.0f && startPoint.y == 0.0f &&
			endPoint.x == 0.0f && endPoint.y == 0.0f)
		{
			continue;
		}

		ImVec2 dir(endPoint.x - startPoint.x, endPoint.y - startPoint.y);
		float len = sqrtf(dir.x * dir.x + dir.y * dir.y);
		ImVec2 trimmedEnd = endPoint;
		if (len > arrowTrim)
		{
			trimmedEnd.x = endPoint.x - dir.x / len * arrowTrim;
			trimmedEnd.y = endPoint.y - dir.y / len * arrowTrim;
		}

		DrawLinkPins(link, startPoint, trimmedEnd);

		ImDrawList* drawList = ed::GetNodeBackgroundDrawList(ed::NodeId(link.fromId));
		if (drawList)
		{
			const ImU32 linkColor = IM_COL32(210, 210, 210, 255);
			DrawArrowHead(drawList, startPoint, endPoint, linkColor);
		}
	}
}
// リンク1本分の接続点を計算する
void StudioChildStateGraph::CalcLinkPoints(const AnimLink& link, ImVec2& outStart, ImVec2& outEnd)
{
	auto fromIt = stateScreenRects.find(link.fromId);
	auto toIt = stateScreenRects.find(link.toId);
	if (fromIt == stateScreenRects.end() || toIt == stateScreenRects.end())
	{
		outStart = ImVec2();
		outEnd = ImVec2();
		return;
	}

	const ImVec2& fromMin = fromIt->second.first;
	const ImVec2& fromMax = fromIt->second.second;
	const ImVec2& toMin = toIt->second.first;
	const ImVec2& toMax = toIt->second.second;

	ImVec2 fromCenter((fromMin.x + fromMax.x) * 0.5f, (fromMin.y + fromMax.y) * 0.5f);
	ImVec2 toCenter((toMin.x + toMax.x) * 0.5f, (toMin.y + toMax.y) * 0.5f);

	ImVec2 startPoint = GetEdgePoint(fromMin, fromMax, toCenter);
	ImVec2 endPoint = GetEdgePoint(toMin, toMax, fromCenter);

	if (HasReverseLink(link.fromId, link.toId))
	{
		bool fromIsSmaller = link.fromId < link.toId;
		const ImVec2& baseCenter = fromIsSmaller ? fromCenter : toCenter;
		const ImVec2& otherCenter = fromIsSmaller ? toCenter : fromCenter;

		ImVec2 dir(otherCenter.x - baseCenter.x, otherCenter.y - baseCenter.y);
		float len = sqrtf(dir.x * dir.x + dir.y * dir.y);
		if (len > 0.0001f)
		{
			ImVec2 perp(-dir.y / len, dir.x / len);

			const float offsetAmount = 8.0f;
			float sign = fromIsSmaller ? 1.0f : -1.0f;

			ImVec2 offset(perp.x * offsetAmount * sign, perp.y * offsetAmount * sign);

			startPoint.x += offset.x;
			startPoint.y += offset.y;
			endPoint.x += offset.x;
			endPoint.y += offset.y;
		}
	}

	outStart = startPoint;
	outEnd = endPoint;
}
// 計算済みの座標にピンを配置する
void StudioChildStateGraph::DrawLinkPins(const AnimLink& link, const ImVec2& start, const ImVec2& end)
{
	const int64_t idOffset = 40000000; // 親(10,000,000)・条件(1M〜4M)・自身のGetPinId(30,000,000)と被らない値
	ed::NodeId outNodeId(idOffset + (int64_t)link.id * 2);
	ed::NodeId inNodeId(idOffset + (int64_t)link.id * 2 + 1);

	ed::PinId outPinId = GetPinId(link.id, true);
	ed::PinId inPinId = GetPinId(link.id, false);

	ed::PushStyleVar(ed::StyleVar_NodePadding, ImVec4(0, 0, 0, 0));
	ed::PushStyleColor(ed::StyleColor_NodeBg, ImColor(0, 0, 0, 0));
	ed::PushStyleColor(ed::StyleColor_NodeBorder, ImColor(0, 0, 0, 0));

	ed::PushStyleVar(ed::StyleVar_LinkStrength, 0.0f);

	ed::SetNodePosition(outNodeId, start);
	ed::BeginNode(outNodeId);
	ed::BeginPin(outPinId, ed::PinKind::Output);
	ImGui::Dummy(ImVec2(1, 1));
	ed::EndPin();
	ed::EndNode();

	ed::SetNodePosition(inNodeId, end);
	ed::BeginNode(inNodeId);
	ed::BeginPin(inPinId, ed::PinKind::Input);
	ImGui::Dummy(ImVec2(1, 1));
	ed::EndPin();
	ed::EndNode();

	ed::PopStyleVar(2);
	ed::PopStyleColor(2);

	ed::Link(ed::LinkId(link.id), outPinId, inPinId);
}
// 辺からリンクを伸ばす
ImVec2 StudioChildStateGraph::GetEdgePoint(const ImVec2& rectMin, const ImVec2& rectMax, const ImVec2& target)
{
	ImVec2 center((rectMin.x + rectMax.x) * 0.5f, (rectMin.y + rectMax.y) * 0.5f);
	float halfW = (rectMax.x - rectMin.x) * 0.5f;
	float halfH = (rectMax.y - rectMin.y) * 0.5f;

	ImVec2 dir(target.x - center.x, target.y - center.y);
	if (dir.x == 0.0f && dir.y == 0.0f) return center;

	float scaleX = (dir.x != 0.0f) ? halfW / fabsf(dir.x) : FLT_MAX;
	float scaleY = (dir.y != 0.0f) ? halfH / fabsf(dir.y) : FLT_MAX;
	float scale = (scaleX < scaleY) ? scaleX : scaleY;

	return ImVec2(center.x + dir.x * scale, center.y + dir.y * scale);
}
// 双方向のLinkがあるか確認する
bool StudioChildStateGraph::HasReverseLink(int fromId, int toId) const
{
	for (const auto& link : graph->childLinks)
	{
		if (link.fromId == toId && link.toId == fromId)
		{
			return true;
		}
	}
	return false;
}
// 矢印描画
void StudioChildStateGraph::DrawArrowHead(ImDrawList* drawList, const ImVec2& start, const ImVec2& end, ImU32 color)
{
	ImVec2 dir(end.x - start.x, end.y - start.y);
	float len = sqrtf(dir.x * dir.x + dir.y * dir.y);
	if (len < 0.0001f) return;

	dir.x /= len;
	dir.y /= len;

	ImVec2 perp(-dir.y, dir.x);

	const float arrowLength = 18.0f;
	const float arrowWidth = 13.0f;

	ImVec2 tip = end;
	ImVec2 baseCenter(end.x - dir.x * arrowLength, end.y - dir.y * arrowLength);
	ImVec2 baseLeft(baseCenter.x + perp.x * arrowWidth * 0.5f, baseCenter.y + perp.y * arrowWidth * 0.5f);
	ImVec2 baseRight(baseCenter.x - perp.x * arrowWidth * 0.5f, baseCenter.y - perp.y * arrowWidth * 0.5f);

	drawList->AddTriangleFilled(tip, baseLeft, baseRight, color);
}


// 右クリックメニュー
void StudioChildStateGraph::DrawCreateStateMenu()
{
	ed::Suspend();
	if (ed::ShowBackgroundContextMenu())
	{
		pendingCreatePos = ImGui::GetMousePos();
		ImGui::OpenPopup("CreateNewChildState");
	}
	if (ImGui::BeginPopup("CreateNewChildState"))
	{
		if (ImGui::MenuItem(u8"State"))
		{
			ImVec2 canvasPos = ed::ScreenToCanvas(pendingCreatePos);
			int newId = graph->CreateChildState("New State", canvasPos.x, canvasPos.y);
			ed::SetNodePosition(ed::NodeId(newId), canvasPos);
		}
		ImGui::EndPopup();
	}
	ed::Resume();
}
// Link作成の起点選択(State右クリック)
void StudioChildStateGraph::DrawStateContextMenu()
{
	static ed::NodeId contextNodeId;

	ed::Suspend();
	if (ed::ShowNodeContextMenu(&contextNodeId))
	{
		ImGui::OpenPopup("ChildNodeContextMenu");
	}
	if (ImGui::BeginPopup("ChildNodeContextMenu"))
	{
		if (ImGui::MenuItem(u8"Link"))
		{
			pendingLinkFromId = (int)contextNodeId.Get();
		}
		ImGui::EndPopup();
	}
	ed::Resume();
}
// Link作成モード中の入力処理
void StudioChildStateGraph::UpdateLinkCreation()
{
	if (pendingLinkFromId == -1) return;

	if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
	{
		ed::NodeId hovered = ed::GetHoveredNode();

		if (hovered && (int)hovered.Get() != pendingLinkFromId)
		{
			int targetId = (int)hovered.Get();

			// Startには他のStateから遷移させられない(常に始点であるべきなので)
			bool targetIsStart = false;
			for (const auto& state : graph->children)
			{
				if (state.id == targetId && state.type == ChildStateType::Start)
				{
					targetIsStart = true;
					break;
				}
			}

			if (!targetIsStart)
			{
				graph->CreateChildLink(pendingLinkFromId, targetId);
			}
		}

		pendingLinkFromId = -1;
	}
}
// Link作成中のプレビュー線描画
void StudioChildStateGraph::DrawPendingLink()
{
	if (pendingLinkFromId == -1) return;

	auto it = stateScreenRects.find(pendingLinkFromId);
	if (it == stateScreenRects.end()) return;

	ImVec2 mousePos = ImGui::GetMousePos();
	ImVec2 startPoint = GetEdgePoint(it->second.first, it->second.second, mousePos);

	ImDrawList* drawList = ed::GetNodeBackgroundDrawList(ed::NodeId(pendingLinkFromId));
	if (!drawList) return;

	const ImU32 previewColor = IM_COL32(255, 200, 80, 255);

	drawList->AddLine(startPoint, mousePos, previewColor, 2.0f);
}


// 選択状態を更新
void StudioChildStateGraph::UpdateSelection()
{
	selectedState = nullptr;
	selectedLink = nullptr;

	if (ed::GetSelectedObjectCount() == 0) return;

	ed::NodeId selectedNodeId;
	if (ed::GetSelectedNodes(&selectedNodeId, 1) > 0)
	{
		for (auto& state : graph->children)
		{
			if (state.id == (int)selectedNodeId.Get())
			{
				selectedState = &state;
				return;
			}
		}
	}

	ed::LinkId selectedLinkId;
	if (ed::GetSelectedLinks(&selectedLinkId, 1) > 0)
	{
		for (auto& link : graph->childLinks)
		{
			if (link.id == (int)selectedLinkId.Get())
			{
				selectedLink = &link;
				return;
			}
		}
	}
}


// State選択時のInspector
void StudioChildStateGraph::DrawStateInspector(ChildState* state)
{
	if (state->type != ChildStateType::Normal)
	{
		ImGui::Text(state->type == ChildStateType::Start ? "Start" : "End");
		ImGui::TextDisabled("This is a unique node and cannot be edited.");
		return;
	}

	char nameBuf[256];
	strncpy_s(nameBuf, state->name.c_str(), sizeof(nameBuf) - 1);
	if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf)))
	{
		state->name = nameBuf;
	}

	// 再生するアニメーションの選択
	const char* previewName = state->clipName.empty() ? "(None)" : state->clipName.c_str();
	if (availableAnimations && ImGui::BeginCombo("Animation", previewName))
	{
		for (const auto& animName : *availableAnimations)
		{
			bool isSelected = (state->clipName == animName);
			if (ImGui::Selectable(animName.c_str(), isSelected))
			{
				state->clipName = animName;
			}
			if (isSelected)
			{
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}

	ImGui::Checkbox("Use Root Motion", &state->useRootMotion);
	ImGui::Checkbox("Blend", &state->blend);

	int stateId = state->id;
	DrawDeleteButton("Delete State", [this, stateId]() { graph->DeleteChildState(stateId); });
}
// Link選択時のInspector
void StudioChildStateGraph::DrawLinkInspector(AnimLink* link)
{
	const ChildState* fromState = nullptr;
	const ChildState* toState = nullptr;
	for (const auto& state : graph->children)
	{
		if (state.id == link->fromId) fromState = &state;
		if (state.id == link->toId)   toState = &state;
	}

	std::string fromName = fromState ? fromState->name : "?";
	std::string toName = toState ? toState->name : "?";

	ImGui::TextWrapped("%s -> %s", fromName.c_str(), toName.c_str());

	if (ImGui::Button("Edit Condition", ImVec2(-1.0f, 0.0f)))
	{
		pendingNavigation = NavigationRequest::EditCondition;
	}

	int linkId = link->id;
	DrawDeleteButton("Delete Link", [this, linkId]() { graph->DeleteChildLink(linkId); });
}


// ノード内で中央寄せにする
void StudioChildStateGraph::CenterNextItem(float totalWidth, float itemWidth)
{
	float pad = (totalWidth - itemWidth) * 0.5f;
	if (pad > 0.0f)
	{
		ImGui::Dummy(ImVec2(pad, 0.0f));
		ImGui::SameLine(0.0f, 0.0f);
	}
}
// 共通の削除ボタン
void StudioChildStateGraph::DrawDeleteButton(const char* label, const std::function<void()>& onDelete)
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
		onDelete();
	}
}