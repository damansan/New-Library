#include "StudioChildStateGraph.h"

void StudioChildStateGraph::Initialize()
{
	canvas.Initialize();
}


// ImGui描画
void StudioChildStateGraph::DrawGUI(ParentState& parentState, const std::function<void(AnimLink*)>& onEditCondition)
{
	parent = &parentState;
	pendingEditCondition = onEditCondition;

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
		DrawLinkInspector(selectedLink, pendingEditCondition);
		return;
	}

	// どちらも選択されていない場合
	ImGui::TextDisabled("No state selected.");
}


// State描画
void StudioChildStateGraph::DrawStates()
{
	stateScreenRects.clear();

	for (auto& state : parent->children)
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
	ed::PopStyleVar(1);
	ed::PopStyleColor(2);
}


// Link描画
void StudioChildStateGraph::DrawLinks()
{
	const float arrowTrim = 10.0f; // 矢印の底辺の位置まで、線を手前に引く量

	for (const auto& link : parent->childLinks)
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
	const int64_t idOffset = 15000000;
	ed::NodeId outNodeId(idOffset + (int64_t)link.id * 2);
	ed::NodeId inNodeId(idOffset + (int64_t)link.id * 2 + 1);

	ed::PinId outPinId = GetOutPinId(link.id);
	ed::PinId inPinId = GetInPinId(link.id);

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
	for (const auto& link : parent->childLinks)
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
		ImGui::OpenPopup("CreateChildState");
	}
	if (ImGui::BeginPopup("CreateChildState"))
	{
		if (ImGui::MenuItem(u8"State"))
		{
			ImVec2 canvasPos = ed::ScreenToCanvas(pendingCreatePos);
			int newId = parent->CreateChildState("New State", canvasPos.x, canvasPos.y);
			ed::SetNodePosition(ed::NodeId(newId), canvasPos);
		}
		ImGui::EndPopup();
	}
	ed::Resume();
}
// Link作成の起点選択(State右クリック)
void StudioChildStateGraph::DrawStateContextMenu()
{
	ed::Suspend();

	ed::NodeId contextNodeId;
	if (ed::ShowNodeContextMenu(&contextNodeId))
	{
		int nodeId = (int)contextNodeId.Get();

		bool isValidSource = false;
		for (const auto& state : parent->children)
		{
			if (state.id == nodeId) { isValidSource = true; break; }
		}

		if (isValidSource)
		{
			ImGui::OpenPopup("ChildStateContextMenu");
			pendingLinkFromId = nodeId;
		}
	}

	if (ImGui::BeginPopup("ChildStateContextMenu"))
	{
		if (ImGui::MenuItem("Link"))
		{
			// UpdateLinkCreationが、pendingLinkFromIdを見て処理を続ける
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

			bool targetIsEntry = false;
			for (const auto& state : parent->children)
			{
				if (state.id == targetId && state.type == ChildStateType::Start)
				{
					targetIsEntry = true;
					break;
				}
			}

			if (!targetIsEntry)
			{
				parent->CreateChildLink(pendingLinkFromId, targetId);
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

	ImVec2 fromMin = it->second.first;
	ImVec2 fromMax = it->second.second;
	ImVec2 fromCenter((fromMin.x + fromMax.x) * 0.5f, (fromMin.y + fromMax.y) * 0.5f);

	ImVec2 mousePos = ImGui::GetMousePos();
	ImVec2 startPoint = GetEdgePoint(fromMin, fromMax, mousePos);

	ImDrawList* drawList = ImGui::GetForegroundDrawList();
	drawList->AddLine(startPoint, mousePos, IM_COL32(210, 210, 210, 180), 2.0f);
}


// 選択状態を更新
void StudioChildStateGraph::UpdateSelection()
{
}
// State選択時のInspector
void StudioChildStateGraph::DrawStateInspector(ChildState* state)
{
}
// Link選択時の Inspector
void StudioChildStateGraph::DrawLinkInspector(AnimLink* link, const std::function<void(AnimLink*)>& onEditCondition)
{
}


// ノード内で中央寄せにする
void StudioChildStateGraph::CenterNextItem(float totalWidth, float itemWidth)
{
}
// 共通の削除ボタン
void StudioChildStateGraph::DrawDeleteButton(const char* label, const std::function<void()>& onDelete)
{
}