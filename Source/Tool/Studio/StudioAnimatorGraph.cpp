#include "StudioAnimatorGraph.h"
#include "ECS/Components/SkinnedMeshComponent.h"
#include <fstream>
#include <json.hpp>

void StudioAnimatorGraph::Initialize()
{
	canvas.Initialize();
}

// ImGui描画
void StudioAnimatorGraph::DrawGUI(entt::registry& registry, StudioContext& context)
{
	if (!context.showAnimatorGraph) return;

	if (!loaded && !context.attachedGraphPath.empty())
	{
		Load(context);
		loaded = true;
		needsApplyPositions = true;
	}

	RefreshAvailableAnimations(registry);

	std::string panelName = "AnimatorGraph##" + std::to_string(windowId);
	ImGui::Begin(panelName.c_str());

	DrawToolbar(context);

	ImGui::Columns(2, "AnimatorGraphColumns", true);

	if (!columnWidthInitialized)
	{
		ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * (3.0f / 4.0f));
		columnWidthInitialized = true;
	}

	if (ImGui::GetColumnWidth(0) > ImGui::GetWindowWidth() - 200.0f)
	{
		ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() - 220.0f);
	}

	canvas.Begin("AnimatorCanvas");

	DrawDropTarget(context);

	DrawNodes();
	DrawLinks();

	DrawCreateNodeMenu();
	DrawNodeContextMenu();
	UpdateLinkCreation();
	DrawPendingLink();

	AnimState* selectedState = GetSelectedState();
	AnimLink* selectedLink = selectedState ? nullptr : GetSelectedLink();

	canvas.End();

	ImGui::NextColumn();

	DrawInspector(selectedState, selectedLink);

	ImGui::Columns(1);

	ImGui::End();
}

// 上側ツールバー
void StudioAnimatorGraph::DrawToolbar(StudioContext& context)
{
	// エラーメッセージ
	if (!attachError.empty())
	{
		ImGui::TextColored(ImVec4(1, 0.4f, 0.4f, 1), "%s", attachError.c_str());
	}

	// 保存ボタン
	if (!context.attachedGraphPath.empty())
	{
		if (ImGui::Button("Save"))
		{
			Save(context);
		}
		ImGui::SameLine();
		ImGui::TextDisabled("%s", context.attachedGraphPath.c_str());
	}
	else
	{
		ImGui::TextDisabled("No graph attached.");
	}
	ImGui::Separator();
}

// .animgraphのD&D受け口
void StudioAnimatorGraph::DrawDropTarget(StudioContext& context)
{
	// パネル全体を.animgraphのドロップ対象にする
	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ANIMGRAPH_PATH"))
		{
			std::string droppedPath(static_cast<const char*>(payload->Data));
			TryAttachGraph(droppedPath, context);
		}
		ImGui::EndDragDropTarget();
	}
}
void StudioAnimatorGraph::TryAttachGraph(const std::string& path, StudioContext& context)
{
	std::ifstream ifs(path);
	if (!ifs.is_open())
	{
		attachError = "Failed to open file.";
		return;
	}

	std::string sourceModelPath;
	try
	{
		nlohmann::json j;
		ifs >> j;
		sourceModelPath = j.value("sourceModelPath", "");
	}
	catch (const std::exception&)
	{
		attachError = "Invalid json.";
		return;
	}

	// 新規作成Jsonは受け入れ、既存Jsonならモデルパスを比較
	if (!sourceModelPath.empty() && sourceModelPath != context.currentModelPath)
	{
		attachError = "This graph belongs to a different model.";
		return;
	}

	attachError.clear();
	context.attachedGraphPath = path;
	loaded = false;
}

// State描画
void StudioAnimatorGraph::DrawNodes()
{
	nodeScreenRects.clear();

	for (auto& state : graph.states)
	{
		ed::NodeId nodeId(state.id);

		if (needsApplyPositions)
		{
			ed::SetNodePosition(nodeId, ImVec2(state.posX, state.posY));
		}

		PushStateStyle(state);

		ed::BeginNode(nodeId);
		if (state.type == AnimStateType::Entry)
		{
			const float totalWidth = 100.0f;
			float textWidth = ImGui::CalcTextSize(state.name.c_str()).x;
			float padding = (totalWidth - textWidth) * 0.5f;

			if (padding > 0.0f)
			{
				ImGui::Dummy(ImVec2(padding, 0.0f));
				ImGui::SameLine(0.0f, 0.0f);
			}

			ImGui::Text("%s", state.name.c_str());

			if (padding > 0.0f)
			{
				ImGui::SameLine(0.0f, 0.0f);
				ImGui::Dummy(ImVec2(padding, 0.0f));
			}
		}
		else
		{
			static const float minWidth = ImGui::CalcTextSize("New State").x;

			float textWidth = ImGui::CalcTextSize(state.name.c_str()).x;
			float padding = (minWidth - textWidth) * 0.5f;

			if (padding > 0.0f)
			{
				ImGui::Dummy(ImVec2(padding, 0.0f));
				ImGui::SameLine(0.0f, 0.0f);
			}

			ImGui::Text("%s", state.name.c_str());

			if (padding > 0.0f)
			{
				ImGui::SameLine(0.0f, 0.0f);
				ImGui::Dummy(ImVec2(padding, 0.0f));
			}
		}
		ed::EndNode();

		nodeScreenRects[state.id] = { ImGui::GetItemRectMin(), ImGui::GetItemRectMax() };

		PopStateStyle(state);

		ImVec2 currentPos = ed::GetNodePosition(nodeId);
		state.posX = currentPos.x;
		state.posY = currentPos.y;
	}
	needsApplyPositions = false;
}
void StudioAnimatorGraph::PushStateStyle(const AnimState& state)
{
	switch (state.type)
	{
	case AnimStateType::Entry:
		ed::PushStyleColor(ed::StyleColor_NodeBg, ImColor(46, 125, 50, 255));
		ed::PushStyleColor(ed::StyleColor_NodeBorder, ImColor(27, 94, 32, 255));
		ed::PushStyleVar(ed::StyleVar_NodeRounding, 4.0f);
		break;

	case AnimStateType::State:
		ed::PushStyleColor(ed::StyleColor_NodeBg, ImColor(state.color[0], state.color[1], state.color[2], 1.0f));
		ed::PushStyleColor(ed::StyleColor_NodeBorder, ImColor(state.color[0] * 0.6f, state.color[1] * 0.6f, state.color[2] * 0.6f, 1.0f));
		ed::PushStyleVar(ed::StyleVar_NodeRounding, 4.0f);
		break;

	default:
		break;
	}
}
void StudioAnimatorGraph::PopStateStyle(const AnimState& state)
{
	switch (state.type)
	{
	case AnimStateType::Entry:
		ed::PopStyleVar(1);
		ed::PopStyleColor(2);
		break;

	case AnimStateType::State:
		ed::PopStyleVar(1);
		ed::PopStyleColor(2);
		break;

	default:
		break;
	}
}

// Link描画
void StudioAnimatorGraph::DrawLinks()
{
	const float arrowTrim = 10.0f; // 矢印の底辺の位置まで、線を手前に引く量

	for (const auto& link : graph.stateLinks)
	{
		ImVec2 startPoint, endPoint;
		CalcLinkPoints(link, startPoint, endPoint);

		if (startPoint.x == 0.0f && startPoint.y == 0.0f &&
			endPoint.x == 0.0f && endPoint.y == 0.0f)
		{
			continue;
		}

		// 線(ピン)の終点だけ、矢印の底辺分だけ手前に引く
		ImVec2 dir(endPoint.x - startPoint.x, endPoint.y - startPoint.y);
		float len = sqrtf(dir.x * dir.x + dir.y * dir.y);
		ImVec2 trimmedEnd = endPoint;
		if (len > arrowTrim)
		{
			trimmedEnd.x = endPoint.x - dir.x / len * arrowTrim;
			trimmedEnd.y = endPoint.y - dir.y / len * arrowTrim;
		}

		// 線(標準機能)は手前で止めた点まで
		DrawLinkPins(link, startPoint, trimmedEnd);

		// 矢印の先端は、本来のStateの辺(end)まで届かせる
		ImDrawList* drawList = ed::GetNodeBackgroundDrawList(ed::NodeId(link.fromId));
		if (drawList)
		{
			const ImU32 linkColor = IM_COL32(210, 210, 210, 255);
			DrawArrowHead(drawList, startPoint, endPoint, linkColor);
		}
	}
}
// リンク1本分の接続点を計算する
void StudioAnimatorGraph::CalcLinkPoints(const AnimLink& link, ImVec2& outStart, ImVec2& outEnd)
{
	auto fromIt = nodeScreenRects.find(link.fromId);
	auto toIt = nodeScreenRects.find(link.toId);
	if (fromIt == nodeScreenRects.end() || toIt == nodeScreenRects.end())
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

	// 接続点を求める
	ImVec2 startPoint = GetEdgePoint(fromMin, fromMax, toCenter);
	ImVec2 endPoint = GetEdgePoint(toMin, toMax, fromCenter);

	// 双方向にリンクが存在する場合、接続点自体を垂直方向へ一定量だけ平行移動する
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
void StudioAnimatorGraph::DrawLinkPins(const AnimLink& link, const ImVec2& start, const ImVec2& end)
{
	const int64_t idOffset = 10000000;
	ed::NodeId outNodeId(idOffset + (int64_t)link.id * 2);
	ed::NodeId inNodeId(idOffset + (int64_t)link.id * 2 + 1);

	ed::PinId outPinId = GetOutPinId(link.id);
	ed::PinId inPinId = GetInPinId(link.id);

	// ノード自体は見た目に出さない(余白・背景・枠線を潰す)
	ed::PushStyleVar(ed::StyleVar_NodePadding, ImVec4(0, 0, 0, 0));
	ed::PushStyleColor(ed::StyleColor_NodeBg, ImColor(0, 0, 0, 0));
	ed::PushStyleColor(ed::StyleColor_NodeBorder, ImColor(0, 0, 0, 0));

	// LinkStrengthはBeginPinの時点で記録されるため、BeginPinより前に設定する
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

	ed::PopStyleVar(2); // LinkStrength, NodePadding
	ed::PopStyleColor(2);

	ed::Link(ed::LinkId(link.id), outPinId, inPinId);
}
// 辺からリンクを伸ばす
ImVec2 StudioAnimatorGraph::GetEdgePoint(const ImVec2& rectMin, const ImVec2& rectMax, const ImVec2& target)
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
bool StudioAnimatorGraph::HasReverseLink(int fromId, int toId) const
{
	for (const auto& link : graph.stateLinks)
	{
		if (link.fromId == toId && link.toId == fromId)
		{
			return true;
		}
	}
	return false;
}
// 矢印描画
void StudioAnimatorGraph::DrawArrowHead(ImDrawList* drawList, const ImVec2& start, const ImVec2& end, ImU32 color)
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
void StudioAnimatorGraph::DrawCreateNodeMenu()
{
	ed::Suspend();
	if (ed::ShowBackgroundContextMenu())
	{
		pendingCreatePos = ImGui::GetMousePos();
		ImGui::OpenPopup("CreateNewState");
	}
	if (ImGui::BeginPopup("CreateNewState"))
	{
		if (ImGui::MenuItem(u8"State"))
		{
			ImVec2 canvasPos = ed::ScreenToCanvas(pendingCreatePos);
			int newId = graph.CreateState("New State", canvasPos.x, canvasPos.y);
			ed::SetNodePosition(ed::NodeId(newId), canvasPos);
		}
		ImGui::EndPopup();
	}
	ed::Resume();
}
void StudioAnimatorGraph::DrawNodeContextMenu()
{
	static ed::NodeId contextNodeId;

	ed::Suspend();
	if (ed::ShowNodeContextMenu(&contextNodeId))
	{
		ImGui::OpenPopup("NodeContextMenu");
	}
	if (ImGui::BeginPopup("NodeContextMenu"))
	{
		if (ImGui::MenuItem(u8"Link"))
		{
			pendingLinkFromId = (int)contextNodeId.Get();
		}
		ImGui::EndPopup();
	}
	ed::Resume();
}
void StudioAnimatorGraph::UpdateLinkCreation()
{
	if (pendingLinkFromId == -1) return;

	if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
	{
		ed::NodeId hovered = ed::GetHoveredNode();

		if (hovered && (int)hovered.Get() != pendingLinkFromId)
		{
			int targetId = (int)hovered.Get();

			// Entryは入力(繋がれる側)を持たない。Entryへの接続は無視する
			bool targetIsEntry = false;
			for (const auto& state : graph.states)
			{
				if (state.id == targetId && state.type == AnimStateType::Entry)
				{
					targetIsEntry = true;
					break;
				}
			}

			if (!targetIsEntry)
			{
				graph.CreateLink(pendingLinkFromId, targetId);
			}
		}

		// ノード上でもノードが無い場所でも、クリックしたらモードを終了する
		pendingLinkFromId = -1;
	}
}
void StudioAnimatorGraph::DrawPendingLink()
{
	if (pendingLinkFromId == -1) return;

	auto it = nodeScreenRects.find(pendingLinkFromId);
	if (it == nodeScreenRects.end()) return;

	ImVec2 mousePos = ImGui::GetMousePos();
	ImVec2 startPoint = GetEdgePoint(it->second.first, it->second.second, mousePos);

	ImDrawList* drawList = ed::GetNodeBackgroundDrawList(ed::NodeId(pendingLinkFromId));
	if (!drawList) return;

	const ImU32 previewColor = IM_COL32(255, 200, 80, 255); // オレンジ

	drawList->AddLine(startPoint, mousePos, previewColor, 2.0f);
}

// 右側メニュー
void StudioAnimatorGraph::DrawInspector(AnimState* selectedState, AnimLink* selectedLink)
{
	// Stateが選択されている場合
	if (selectedState)
	{
		if (selectedState->type == AnimStateType::Entry)
		{
			ImGui::Text("Entry");
			ImGui::TextDisabled("This is a unique node and cannot be edited.");
			return;
		}

		char nameBuf[256];
		strncpy_s(nameBuf, selectedState->name.c_str(), sizeof(nameBuf) - 1);
		if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf)))
		{
			selectedState->name = nameBuf;
		}

		ImGui::ColorEdit3("Color", selectedState->color);

		ImGui::Spacing();
		ImGui::Spacing();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.15f, 0.15f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.2f, 0.2f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));

		bool pressed = ImGui::Button("Delete State", ImVec2(-1.0f, 0.0f));

		ImGui::PopStyleColor(3);

		if (pressed)
		{
			graph.DeleteState(selectedState->id);
		}
		return;
	}

	// Linkが選択されている場合
	if (selectedLink)
	{
		const AnimState* fromState = nullptr;
		const AnimState* toState = nullptr;
		for (const auto& state : graph.states)
		{
			if (state.id == selectedLink->fromId) fromState = &state;
			if (state.id == selectedLink->toId)   toState = &state;
		}

		std::string fromName = fromState ? fromState->name : "?";
		std::string toName = toState ? toState->name : "?";

		ImGui::Text("Transition");
		ImGui::Separator();
		ImGui::TextWrapped("%s -> %s", fromName.c_str(), toName.c_str());

		// 遷移条件は未実装のため、今は名前の表示と削除のみ対応
		ImGui::TextDisabled("Conditions are not implemented yet.");

		ImGui::Spacing();
		ImGui::Spacing();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.15f, 0.15f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.2f, 0.2f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));

		bool pressed = ImGui::Button("Delete Link", ImVec2(-1.0f, 0.0f));

		ImGui::PopStyleColor(3);

		if (pressed)
		{
			graph.DeleteLink(selectedLink->id);
		}
		return;
	}

	// どちらも選択されていない場合
	ImGui::TextDisabled("No state selected.");
}
AnimState* StudioAnimatorGraph::GetSelectedState()
{
	if (ed::GetSelectedObjectCount() == 0) return nullptr;

	ed::NodeId selectedId;
	ed::GetSelectedNodes(&selectedId, 1);

	for (auto& state : graph.states)
	{
		if (state.id == (int)selectedId.Get())
		{
			return &state;
		}
	}

	return nullptr;
}
AnimLink* StudioAnimatorGraph::GetSelectedLink()
{
	ed::LinkId selectedId;
	if (ed::GetSelectedLinks(&selectedId, 1) == 0) return nullptr;

	for (auto& link : graph.stateLinks)
	{
		if (link.id == (int)selectedId.Get())
		{
			return &link;
		}
	}

	return nullptr;
}

void StudioAnimatorGraph::RefreshAvailableAnimations(entt::registry& registry)
{
	availableAnimations.clear();

	auto view = registry.view<SkinnedMeshComponent>();
	if (view.begin() == view.end()) return;

	auto entity = *view.begin();
	auto& skinned = view.get<SkinnedMeshComponent>(entity);

	if (!skinned.model) return;

	for (const auto& anim : skinned.model->GetAnimations())
	{
		availableAnimations.push_back(anim.name);
	}
}

// 保存
void StudioAnimatorGraph::Save(StudioContext& context)
{
	if (context.attachedGraphPath.empty()) return;

	nlohmann::ordered_json j;
	j["sourceModelPath"] = context.currentModelPath;

	nlohmann::ordered_json graphJson = graph.ToJson();
	j["states"] = graphJson["states"];
	j["stateLinks"] = graphJson["stateLinks"];

	std::ofstream ofs(context.attachedGraphPath);
	if (ofs.is_open())
	{
		ofs << j.dump(4);
	}
}
// 読み込み
void StudioAnimatorGraph::Load(StudioContext& context)
{
	std::ifstream ifs(context.attachedGraphPath);
	if (!ifs.is_open()) return;

	try
	{
		nlohmann::json j;
		ifs >> j;
		graph.FromJson(j);
	}
	catch (const std::exception&)
	{
		// 壊れているか空のjsonの場合なにもしない
	}
}