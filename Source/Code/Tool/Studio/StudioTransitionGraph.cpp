#include "StudioTransitionGraph.h"

void StudioTransitionGraph::Initialize()
{
	canvas.Initialize();
}

// ImGui描画(State+Linkの描画本体)
void StudioTransitionGraph::DrawGUI(AnimatorGraphData& g)
{
	graph = &g;

	canvas.Begin("TransitionCanvas");

	DrawStates();
	DrawLinks();

	DrawStateContextMenu();
	UpdateLinkCreation();
	DrawPendingLink();

	UpdateSelection();

	canvas.End();
}

// 右側メニュー
void StudioTransitionGraph::DrawInspector()
{
	if (selectedState)
	{
		// Stateは読み取り専用(名前・色の編集、削除ボタンは持たない)
		ImGui::Text("%s", selectedState->name.c_str());
		ImGui::TextDisabled("State name/color can only be edited from the Animator Graph.");
		return;
	}

	if (selectedLink)
	{
		DrawLinkInspector(selectedLink);
		return;
	}

	ImGui::TextDisabled("No link selected.");
}

// 条件式編集画面への遷移要求
bool StudioTransitionGraph::ConsumeConditionEditRequest(TransitionLink*& outLink)
{
	if (!pendingConditionEdit)
	{
		outLink = nullptr;
		return false;
	}

	outLink = selectedLink;
	pendingConditionEdit = false;
	return true;
}

// State描画
void StudioTransitionGraph::DrawStates()
{
	stateScreenRects.clear();

	// 紫基調のスタイル
	const ImColor bgColor(90, 60, 110, 255);
	const ImColor borderColor(140, 90, 170, 255);

	for (auto& state : graph->parentGraph.states)
	{
		ed::NodeId nodeId(state.id);

		if (needsApplyPositions)
		{
			ed::SetNodePosition(nodeId, ImVec2(state.posX, state.posY));
		}

		ed::PushStyleColor(ed::StyleColor_NodeBg, bgColor);
		ed::PushStyleColor(ed::StyleColor_NodeBorder, borderColor);
		ed::PushStyleVar(ed::StyleVar_NodeRounding, 4.0f);

		ed::BeginNode(nodeId);

		float nodeWidth = (state.type == ParentStateType::Entry) ? 100.0f : ImGui::CalcTextSize("New State").x;
		float textWidth = ImGui::CalcTextSize(state.name.c_str()).x;

		CenterNextItem(nodeWidth, textWidth);
		ImGui::Text("%s", state.name.c_str());

		ImGui::SameLine(nodeWidth);
		ImGui::Dummy(ImVec2(0.0f, 0.0f));

		ed::EndNode();

		stateScreenRects[state.id] = { ImGui::GetItemRectMin(), ImGui::GetItemRectMax() };

		ed::PopStyleVar(1);
		ed::PopStyleColor(2);

		ImVec2 currentPos = ed::GetNodePosition(nodeId);
		state.posX = currentPos.x;
		state.posY = currentPos.y;
	}
	needsApplyPositions = false;
}

// Link描画
void StudioTransitionGraph::DrawLinks()
{
	const float arrowTrim = 10.0f;

	for (const auto& link : graph->transitionGraph.links)
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
			// 遷移用リンクは紫色で描画する
			const ImU32 linkColor = IM_COL32(180, 130, 220, 255);
			DrawArrowHead(drawList, startPoint, endPoint, linkColor);
		}
	}
}
// リンク1本の接続点を計算
void StudioTransitionGraph::CalcLinkPoints(const TransitionLink& link, ImVec2& outStart, ImVec2& outEnd)
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
// 計算済みの座標にピンを配置
void StudioTransitionGraph::DrawLinkPins(const TransitionLink& link, const ImVec2& start, const ImVec2& end)
{
	const int64_t idOffset = 10000000;
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
// 辺からリンクを引き出す点を求める
ImVec2 StudioTransitionGraph::GetEdgePoint(const ImVec2& rectMin, const ImVec2& rectMax, const ImVec2& target)
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
// 逆向きのLinkがあるか確認
bool StudioTransitionGraph::HasReverseLink(int fromId, int toId) const
{
	for (const auto& link : graph->transitionGraph.links)
	{
		if (link.fromId == toId && link.toId == fromId)
		{
			return true;
		}
	}
	return false;
}
// 矢印描画
void StudioTransitionGraph::DrawArrowHead(ImDrawList* drawList, const ImVec2& start, const ImVec2& end, ImU32 color)
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

// Link作成の起点選択
void StudioTransitionGraph::DrawStateContextMenu()
{
	static ed::NodeId contextNodeId;

	ed::Suspend();
	if (ed::ShowNodeContextMenu(&contextNodeId))
	{
		ImGui::OpenPopup("TransitionNodeContextMenu");
	}
	if (ImGui::BeginPopup("TransitionNodeContextMenu"))
	{
		if (ImGui::MenuItem(u8"Link"))
		{
			pendingLinkFromId = (int)contextNodeId.Get();
		}
		ImGui::EndPopup();
	}
	ed::Resume();
}
// Link作成モードの入力処理
void StudioTransitionGraph::UpdateLinkCreation()
{
	if (pendingLinkFromId == -1) return;

	if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
	{
		ed::NodeId hovered = ed::GetHoveredNode();

		if (hovered && (int)hovered.Get() != pendingLinkFromId)
		{
			int targetId = (int)hovered.Get();

			bool targetIsEntry = false;
			for (const auto& state : graph->parentGraph.states)
			{
				if (state.id == targetId && state.type == ParentStateType::Entry)
				{
					targetIsEntry = true;
					break;
				}
			}

			if (!targetIsEntry)
			{
				graph->transitionGraph.CreateLink(pendingLinkFromId, targetId);
			}
		}

		pendingLinkFromId = -1;
	}
}
// Link作成中のプレビュー描画
void StudioTransitionGraph::DrawPendingLink()
{
	if (pendingLinkFromId == -1) return;

	auto it = stateScreenRects.find(pendingLinkFromId);
	if (it == stateScreenRects.end()) return;

	ImVec2 mousePos = ImGui::GetMousePos();
	ImVec2 startPoint = GetEdgePoint(it->second.first, it->second.second, mousePos);

	ImDrawList* drawList = ed::GetNodeBackgroundDrawList(ed::NodeId(pendingLinkFromId));
	if (!drawList) return;

	const ImU32 previewColor = IM_COL32(200, 150, 255, 255);

	drawList->AddLine(startPoint, mousePos, previewColor, 2.0f);
}

// 選択状態を更新
void StudioTransitionGraph::UpdateSelection()
{
	selectedState = nullptr;
	selectedLink = nullptr;

	if (ed::GetSelectedObjectCount() == 0) return;

	ed::NodeId selectedNodeId;
	if (ed::GetSelectedNodes(&selectedNodeId, 1) > 0)
	{
		for (auto& state : graph->parentGraph.states)
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
		for (auto& link : graph->transitionGraph.links)
		{
			if (link.id == (int)selectedLinkId.Get())
			{
				selectedLink = &link;
				return;
			}
		}
	}
}
// LinkのInspector
void StudioTransitionGraph::DrawLinkInspector(TransitionLink* link)
{
	const ParentState* fromState = nullptr;
	const ParentState* toState = nullptr;
	for (const auto& state : graph->parentGraph.states)
	{
		if (state.id == link->fromId) fromState = &state;
		if (state.id == link->toId)   toState = &state;
	}

	std::string fromName = fromState ? fromState->name : "?";
	std::string toName = toState ? toState->name : "?";

	ImGui::TextWrapped("%s -> %s", fromName.c_str(), toName.c_str());

	if (ImGui::Button("Edit Condition", ImVec2(-1.0f, 0.0f)))
	{
		pendingConditionEdit = true;
	}

	int linkId = link->id;
	DrawDeleteButton("Delete Link", [this, linkId]() { graph->transitionGraph.DeleteLink(linkId); });
}

// ノード内で中央寄せにする
void StudioTransitionGraph::CenterNextItem(float totalWidth, float itemWidth)
{
	float pad = (totalWidth - itemWidth) * 0.5f;
	if (pad > 0.0f)
	{
		ImGui::Dummy(ImVec2(pad, 0.0f));
		ImGui::SameLine(0.0f, 0.0f);
	}
}
// 共通の削除ボタン
void StudioTransitionGraph::DrawDeleteButton(const char* label, const std::function<void()>& onDelete)
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