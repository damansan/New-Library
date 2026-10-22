#include "StudioBlendCanvas.h"
#include <algorithm>
#include <cctype>
#include <cmath>

#include "Engine/ECS/Reflection.h"
#include "StudioStateGraphDraw.h"

namespace
{
	const ImVec4 errorTextColor(1.0f, 0.4f, 0.4f, 1.0f);

	// クリックの判定の距離
	const float pairDotHitDistance = 8.0f; // ペアの真ん中の点
	const float lineHitDistance = 6.0f;    // 線

	// 矩形の中心
	ImVec2 RectCenter(const std::pair<ImVec2, ImVec2>& rect)
	{
		return ImVec2((rect.first.x + rect.second.x) * 0.5f, (rect.first.y + rect.second.y) * 0.5f);
	}

	// 2点の距離
	float Distance(const ImVec2& a, const ImVec2& b)
	{
		float dx = a.x - b.x;
		float dy = a.y - b.y;
		return std::sqrt(dx * dx + dy * dy);
	}

	// 値の型の表示名
	const char* GetValueTypeName(ValueType type)
	{
		switch (type)
		{
		case ValueType::Int:    return "Int";
		case ValueType::Float:  return "Float";
		case ValueType::Bool:   return "Bool";
		case ValueType::Float2: return "Float2";
		case ValueType::Float3: return "Float3";
		case ValueType::Float4: return "Float4";
		default:                return "Unsupported";
		}
	}

	// 小文字にする
	std::string ToLower(std::string s)
	{
		std::transform(s.begin(), s.end(), s.begin(),
			[](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
		return s;
	}
}

void StudioBlendCanvas::Initialize()
{
	// ノードエディタのコンテキストを作る
	canvas.Initialize();
}

// 編集する Blend を探す
BlendNode* StudioBlendCanvas::FindBlendNode(StateMachineData& data, int stateId, int blendId)
{
	StateNode* state = data.FindState(stateId);
	if (!state) return nullptr;

	for (auto& b : state->child.blends)
	{
		if (b.id == blendId) return &b;
	}
	return nullptr;
}

// キャンバスとインスペクターを描く
void StudioBlendCanvas::Draw(BlendNode& blendNode, const std::vector<std::string>& animations)
{
	// 描画中だけ、受け取ったデータを指す
	blend = &blendNode;
	graph = &blendNode.graph;

	ImGui::Columns(2, "BlendColumns", true);
	if (!columnWidthInitialized)
	{
		ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * 0.65f);
		columnWidthInitialized = true;
	}

	// キャンバス
	canvas.Begin("BlendCanvas");

	// 削除の後は、ノードエディタ側に残った選択を消す
	if (clearSelectionRequested)
	{
		ed::ClearSelection();
		clearSelectionRequested = false;
	}

	// ノードの矩形と線は毎フレーム作り直す
	nodeRects.clear();
	pairMidpoints.clear();
	pairSegments.clear();
	outputSegments.clear();

	// ノード
	DrawOutputNode();
	DrawSourceNodes();

	// 位置の反映は、開いた直後の1フレームだけ
	needsApplyPositions = false;

	// 線
	DrawPairLinks();
	DrawOutputLinks();
	DrawPendingLink();

	// メニュー
	DrawBackgroundMenu();
	DrawNodeMenu();

	// 入力
	UpdateLinkCreation();
	UpdateSelection();

	canvas.End();

	ImGui::NextColumn();

	// 右：インスペクター
	DrawInspector(animations);

	// 選んでいる物の削除
	if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
		!ImGui::GetIO().WantTextInput &&
		ImGui::IsKeyPressed(ImGuiKey_Delete))
	{
		DeleteSelection();
	}

	ImGui::Columns(1);
}


// Output ノード
void StudioBlendCanvas::DrawOutputNode()
{
	BlendOutputNode& output = graph->outputNode;
	ed::NodeId nodeId(static_cast<uint64_t>(output.id));

	if (needsApplyPositions)
	{
		ed::SetNodePosition(nodeId, ImVec2(output.editorPos.x, output.editorPos.y));
	}

	// 何もつながっていなければ、枠を赤くする
	bool connected = !graph->outputLinks.empty();

	// Output はオレンジで描く
	ed::PushStyleColor(ed::StyleColor_NodeBg, ImColor(191, 87, 0, 255));
	ed::PushStyleColor(ed::StyleColor_NodeBorder, connected ? ImColor(255, 160, 0, 255) : ImColor(230, 90, 90, 255));
	ed::BeginNode(nodeId);
	ImGui::Text("Output");
	ImGui::Dummy(ImVec2(80.0f, 0.0f));
	ed::EndNode();
	ed::PopStyleColor(2);

	nodeRects[output.id] = { ImGui::GetItemRectMin(), ImGui::GetItemRectMax() };

	// 動かしたら、データの位置に書き戻す
	ImVec2 pos = ed::GetNodePosition(nodeId);
	output.editorPos = { pos.x, pos.y };
}

// 材料ノード
void StudioBlendCanvas::DrawSourceNodes()
{
	for (auto& s : graph->sources)
	{
		ed::NodeId nodeId(static_cast<uint64_t>(s.id));

		if (needsApplyPositions)
		{
			ed::SetNodePosition(nodeId, ImVec2(s.editorPos.x, s.editorPos.y));
		}

		ed::BeginNode(nodeId);
		ImGui::Text("%s", s.name.empty() ? "Source" : s.name.c_str());
		// クリップ(未設定なら灰色で表示)
		ImGui::TextDisabled("%s", s.clipName.empty() ? "(no clip)" : s.clipName.c_str());
		ImGui::Dummy(ImVec2(80.0f, 0.0f));
		ed::EndNode();

		nodeRects[s.id] = { ImGui::GetItemRectMin(), ImGui::GetItemRectMax() };

		// 動かしたら、データの位置に書き戻す
		ImVec2 pos = ed::GetNodePosition(nodeId);
		s.editorPos = { pos.x, pos.y };
	}
}

// ペアの線
void StudioBlendCanvas::DrawPairLinks()
{
	ImDrawList* drawList = ImGui::GetWindowDrawList();

	for (const auto& p : graph->pairLinks)
	{
		auto aIt = nodeRects.find(p.fromId);
		auto bIt = nodeRects.find(p.toId);
		if (aIt == nodeRects.end() || bIt == nodeRects.end()) continue;

		// お互いの中心に向けた、縁の点
		ImVec2 aCenter = RectCenter(aIt->second);
		ImVec2 bCenter = RectCenter(bIt->second);
		ImVec2 start = StateGraphDraw::GetEdgePoint(aIt->second.first, aIt->second.second, bCenter);
		ImVec2 end = StateGraphDraw::GetEdgePoint(bIt->second.first, bIt->second.second, aCenter);
		ImVec2 mid((start.x + end.x) * 0.5f, (start.y + end.y) * 0.5f);

		bool selected = (selectedKind == SelectionKind::Pair && selectedId == p.id);
		ImU32 color = selected ? IM_COL32(255, 200, 80, 255) : IM_COL32(120, 200, 255, 255);

		// 線と、真ん中の点
		drawList->AddLine(start, end, color, selected ? 3.0f : 2.0f);
		drawList->AddCircleFilled(mid, 6.0f, color);

		pairMidpoints[p.id] = mid;
		pairSegments.push_back({ p.id, start, end });
	}
}

// Output への線
void StudioBlendCanvas::DrawOutputLinks()
{
	ImDrawList* drawList = ImGui::GetWindowDrawList();

	auto outIt = nodeRects.find(graph->outputNode.id);
	if (outIt == nodeRects.end()) return;
	ImVec2 outCenter = RectCenter(outIt->second);

	for (const auto& l : graph->outputLinks)
	{
		ImVec2 start;
		if (!CalcLinkStart(l.fromId, outCenter, start)) continue;
		ImVec2 end = StateGraphDraw::GetEdgePoint(outIt->second.first, outIt->second.second, start);

		bool selected = (selectedKind == SelectionKind::OutputLink && selectedId == l.id);
		bool invalid = (!l.always && l.condition.IsEmpty());

		// 正常(白色) 条件なし(赤色) 選択中(黄色)
		ImU32 color = IM_COL32(210, 210, 210, 255);
		if (invalid)  color = IM_COL32(230, 90, 90, 255);
		if (selected) color = IM_COL32(255, 200, 80, 255);

		StateGraphDraw::DrawArrow(drawList, start, end, color, selected ? 3.0f : 2.0f);
		outputSegments.push_back({ l.id, start, end });
	}
}

// 作成中のリンクの線
void StudioBlendCanvas::DrawPendingLink()
{
	if (pendingLinkFromId == -1) return;

	// 元から、マウスの位置まで
	ImVec2 mouse = ImGui::GetMousePos();
	ImVec2 start;
	if (!CalcLinkStart(pendingLinkFromId, mouse, start)) return;

	StateGraphDraw::DrawArrow(ImGui::GetWindowDrawList(), start, mouse, IM_COL32(255, 200, 80, 255), 2.0f);
}

// 背景右クリックのメニュー
void StudioBlendCanvas::DrawBackgroundMenu()
{
	ed::Suspend();

	if (ed::ShowBackgroundContextMenu())
	{
		pendingCreatePos = ImGui::GetMousePos();

		// ペアは「ノード」ではないので、右クリックした位置にペアがあるかを自分で調べる
		contextPairId = HitPair(ed::ScreenToCanvas(pendingCreatePos));
		ImGui::OpenPopup(contextPairId != -1 ? "BlendPairMenu" : "BlendBackgroundMenu");
	}

	// 背景：材料を作る
	if (ImGui::BeginPopup("BlendBackgroundMenu"))
	{
		if (ImGui::MenuItem("Source"))
		{
			ImVec2 canvasPos = ed::ScreenToCanvas(pendingCreatePos);
			int newId = graph->AddSource({ canvasPos.x, canvasPos.y });
			ed::SetNodePosition(ed::NodeId(static_cast<uint64_t>(newId)), canvasPos);
		}
		ImGui::EndPopup();
	}

	// ペア：リンクの作成を始める
	if (ImGui::BeginPopup("BlendPairMenu"))
	{
		if (ImGui::MenuItem("Link"))
		{
			pendingLinkFromId = contextPairId;
		}
		ImGui::EndPopup();
	}

	ed::Resume();
}


// ノード右クリックのメニュー
void StudioBlendCanvas::DrawNodeMenu()
{
	ed::Suspend();

	ed::NodeId nodeId;
	if (ed::ShowNodeContextMenu(&nodeId))
	{
		contextNodeId = static_cast<int>(nodeId.Get());
		ImGui::OpenPopup("BlendNodeMenu");
	}

	if (ImGui::BeginPopup("BlendNodeMenu"))
	{
		if (graph->GetNodeKind(contextNodeId) == BlendNodeKind::Source)
		{
			// リンクの作成を始める
			if (ImGui::MenuItem("Link"))
			{
				pendingLinkFromId = contextNodeId;
			}
		}
		else
		{
			ImGui::TextDisabled("Output");
		}
		ImGui::EndPopup();
	}

	ed::Resume();
}

// リンクの作成中の入力
void StudioBlendCanvas::UpdateLinkCreation()
{
	if (pendingLinkFromId == -1) return;

	// Esc で取り消し
	if (ImGui::IsKeyPressed(ImGuiKey_Escape))
	{
		pendingLinkFromId = -1;
		return;
	}

	if (!ImGui::IsMouseClicked(ImGuiMouseButton_Left)) return;

	// クリックしたノードによって、作る物を変える
	ed::NodeId hovered = ed::GetHoveredNode();
	if (hovered)
	{
		int toId = static_cast<int>(hovered.Get());
		BlendNodeKind fromKind = graph->GetNodeKind(pendingLinkFromId);
		BlendNodeKind toKind = graph->GetNodeKind(toId);

		SelectionKind newKind = SelectionKind::None;
		int newId = -1;

		if (toKind == BlendNodeKind::Output)
		{
			// 材料かペア → Output：Output への線
			newId = graph->AddOutputLink(pendingLinkFromId);
			newKind = SelectionKind::OutputLink;
		}
		else if (fromKind == BlendNodeKind::Source && toKind == BlendNodeKind::Source)
		{
			// 材料 → 材料：ペア
			newId = graph->AddPairLink(pendingLinkFromId, toId);
			newKind = SelectionKind::Pair;
		}

		// 作った物を選択状態にする
		if (newId != -1)
		{
			ClearSelection();
			selectedKind = newKind;
			selectedId = newId;
		}
	}

	// ノード以外をクリックした場合も、作成は終わる
	pendingLinkFromId = -1;
}

// 選択の更新
void StudioBlendCanvas::UpdateSelection()
{
	// ノードの選択
	int count = ed::GetSelectedObjectCount();
	std::vector<ed::NodeId> nodeIds(count > 0 ? count : 1);
	int nodeCount = (count > 0) ? ed::GetSelectedNodes(nodeIds.data(), count) : 0;

	int selectedNode = -1;
	for (int i = 0; i < nodeCount; ++i)
	{
		int id = static_cast<int>(nodeIds[i].Get());
		BlendNodeKind kind = graph->GetNodeKind(id);
		if (kind == BlendNodeKind::Output || kind == BlendNodeKind::Source)
		{
			selectedNode = id;
			break;
		}
	}

	if (selectedNode != -1)
	{
		selectedKind = SelectionKind::Node;
		selectedId = selectedNode;
	}
	else if (selectedKind == SelectionKind::Node)
	{
		// ノードの選択が外れた
		selectedKind = SelectionKind::None;
		selectedId = -1;
	}

	// 線のクリックの判定
	if (pendingLinkFromId != -1) return;
	if (!ed::IsBackgroundClicked()) return;

	ImVec2 mouse = ImGui::GetMousePos();

	// ペアの真ん中の点を優先する
	for (const auto& [pairId, mid] : pairMidpoints)
	{
		if (Distance(mouse, mid) <= pairDotHitDistance)
		{
			selectedKind = SelectionKind::Pair;
			selectedId = pairId;
			ed::ClearSelection();
			return;
		}
	}

	// 一番近い線を探す
	SelectionKind hitKind = SelectionKind::None;
	int hitId = -1;
	float best = lineHitDistance;
	for (const auto& seg : outputSegments)
	{
		float d = StateGraphDraw::DistanceToSegment(mouse, seg.start, seg.end);
		if (d < best) { best = d; hitKind = SelectionKind::OutputLink; hitId = seg.id; }
	}
	for (const auto& seg : pairSegments)
	{
		float d = StateGraphDraw::DistanceToSegment(mouse, seg.start, seg.end);
		if (d < best) { best = d; hitKind = SelectionKind::Pair; hitId = seg.id; }
	}

	// 線の無いところをクリックしたら選択解除
	selectedKind = hitKind;
	selectedId = hitId;
	if (hitId != -1) ed::ClearSelection();
}


// 選んでいる物を削除
void StudioBlendCanvas::DeleteSelection()
{
	switch (selectedKind)
	{
	case SelectionKind::Node:
		// Output は消せない
		if (graph->GetNodeKind(selectedId) == BlendNodeKind::Source) graph->RemoveSource(selectedId);
		break;
	case SelectionKind::Pair:
		graph->RemovePairLink(selectedId);
		break;
	case SelectionKind::OutputLink:
		graph->RemoveOutputLink(selectedId);
		break;
	default:
		break;
	}
	ClearSelection();
}

// 選択を外す
void StudioBlendCanvas::ClearSelection()
{
	selectedKind = SelectionKind::None;
	selectedId = -1;
	clearSelectionRequested = true; // ノードエディタ側の選択も、次に描くときに外す
}

// その位置にあるペアを探す
int StudioBlendCanvas::HitPair(const ImVec2& canvasPos) const
{
	// 真ん中の点を優先する
	for (const auto& [pairId, mid] : pairMidpoints)
	{
		if (Distance(canvasPos, mid) <= pairDotHitDistance) return pairId;
	}

	// 線の近く
	int hitId = -1;
	float best = lineHitDistance;
	for (const auto& seg : pairSegments)
	{
		float d = StateGraphDraw::DistanceToSegment(canvasPos, seg.start, seg.end);
		if (d < best) { best = d; hitId = seg.id; }
	}
	return hitId;
}


void StudioBlendCanvas::DrawInspector(const std::vector<std::string>& animations)
{
	switch (selectedKind)
	{
	case SelectionKind::Node:
		if (selectedId == graph->outputNode.id)
		{
			DrawOutputInspector();
			return;
		}
		for (auto& s : graph->sources)
		{
			if (s.id == selectedId)
			{
				DrawSourceInspector(s, animations);
				return;
			}
		}
		break;

	case SelectionKind::Pair:
		if (BlendPairLink* p = graph->FindPairLink(selectedId))
		{
			DrawPairInspector(*p);
			return;
		}
		break;

	case SelectionKind::OutputLink:
		if (BlendOutputLink* l = graph->FindOutputLink(selectedId))
		{
			DrawOutputLinkInspector(*l);
			return;
		}
		break;

	default:
		break;
	}

	// 未選択
	ImGui::TextDisabled("Nothing selected.");
}

// Output の設定
void StudioBlendCanvas::DrawOutputInspector()
{
	ImGui::Text("Output");
	ImGui::Separator();
	ImGui::TextWrapped("The first link whose condition is true is played.");
	ImGui::Spacing();

	DrawOutputLinkList();
}

// 材料の設定
void StudioBlendCanvas::DrawSourceInspector(BlendSourceNode& source, const std::vector<std::string>& animations)
{
	ImGui::Text("Source");
	ImGui::Separator();

	// 選ぶ材料が変わったら、入力欄を今の名前で作り直す
	if (nameEditingSourceId != source.id)
	{
		nameEditingSourceId = source.id;
		strncpy_s(nameBuffer, source.name.c_str(), sizeof(nameBuffer) - 1);
	}

	// 名前
	if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer), ImGuiInputTextFlags_EnterReturnsTrue))
	{
		source.name = nameBuffer;
	}
	// 入力欄から離れたら、確定していない文字は今の名前に戻す
	if (ImGui::IsItemDeactivated() && source.name != nameBuffer)
	{
		strncpy_s(nameBuffer, source.name.c_str(), sizeof(nameBuffer) - 1);
	}
	ImGui::Separator();

	// 再生するクリップ
	DrawAnimationList(source, animations);
	ImGui::Separator();

	// 削除
	if (StateGraphDraw::DeleteButton("Delete Source"))
	{
		DeleteSelection();
	}
}

// ペアの設定
void StudioBlendCanvas::DrawPairInspector(BlendPairLink& pair)
{
	ImGui::Text("Pair");
	ImGui::Separator();

	ImGui::Text("A: %s", GetSourceName(pair.fromId).c_str());
	ImGui::Text("B: %s", GetSourceName(pair.toId).c_str());
	ImGui::Separator();

	// 比率をどこから取るか
	ImGui::Text("Ratio of B");
	if (ImGui::RadioButton("Fixed", pair.rateSource == BlendRateSource::Fixed))
	{
		pair.rateSource = BlendRateSource::Fixed;
	}
	ImGui::SameLine();
	if (ImGui::RadioButton("Variable", pair.rateSource == BlendRateSource::Variable))
	{
		pair.rateSource = BlendRateSource::Variable;
	}

	if (pair.rateSource == BlendRateSource::Fixed)
	{
		ImGui::SliderFloat("B Ratio", &pair.fixedRate, 0.0f, 1.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
		ImGui::TextDisabled("%s %d%% / %s %d%%",
			GetSourceName(pair.fromId).c_str(), static_cast<int>(std::round((1.0f - pair.fixedRate) * 100.0f)),
			GetSourceName(pair.toId).c_str(), static_cast<int>(std::round(pair.fixedRate * 100.0f)));
	}
	else
	{
		DrawRateVariablePicker(pair);
		ImGui::TextDisabled("0 = A only, 1 = B only (clamped)");
	}
	ImGui::Separator();

	// 削除
	if (StateGraphDraw::DeleteButton("Delete Pair"))
	{
		DeleteSelection();
	}
}

// Output への線の設定
void StudioBlendCanvas::DrawOutputLinkInspector(BlendOutputLink& link)
{
	ImGui::Text("Output Link");
	ImGui::Separator();

	ImGui::Text("%s  ->  Output", GetFromName(link.fromId).c_str());

	// 優先度
	int index = 0;
	for (int i = 0; i < static_cast<int>(graph->outputLinks.size()); ++i)
	{
		if (graph->outputLinks[i].id == link.id) index = i;
	}
	ImGui::Text("Priority: %d / %d", index + 1, static_cast<int>(graph->outputLinks.size()));
	ImGui::Spacing();

	// 常に成立
	ImGui::Checkbox("Always", &link.always);

	// 条件の状態
	if (link.always)
	{
		ImGui::TextDisabled("Condition: Always");
	}
	else if (link.condition.IsEmpty())
	{
		ImGui::TextColored(errorTextColor, "Condition: Empty");
	}
	else
	{
		ImGui::Text("Condition: Set");
	}

	// 条件の編集へ
	if (ImGui::Button("Edit Condition", ImVec2(-1.0f, 0.0f)))
	{
		editConditionRequest = link.id;
	}

	// 削除
	if (StateGraphDraw::DeleteButton("Delete Link"))
	{
		DeleteSelection();
		return;
	}
}

// 材料のアニメーション一覧
void StudioBlendCanvas::DrawAnimationList(BlendSourceNode& source, const std::vector<std::string>& animations)
{
	ImGui::Text("Animation");

	// 一覧を表示する材料が変わったら、今のアニメーションの位置へスクロールする
	if (animationListSourceId != source.id)
	{
		animationListSourceId = source.id;
		scrollToCurrentAnimation = true;
	}

	// 設定されている名前が、モデルのアニメーションに無い時
	bool missing = !source.clipName.empty() &&
		std::find(animations.begin(), animations.end(), source.clipName) == animations.end();
	if (missing)
	{
		ImGui::TextColored(errorTextColor, "%s (missing)", source.clipName.c_str());
	}

	std::string filter = ToLower(animationFilter);

	ImGui::BeginChild("##BlendAnimationList", ImVec2(0.0f, 200.0f), true);
	for (const auto& name : animations)
	{
		if (!filter.empty() && ToLower(name).find(filter) == std::string::npos) continue;

		bool current = (name == source.clipName);
		if (ImGui::Selectable(name.c_str(), current))
		{
			source.clipName = name;
		}
		if (current && scrollToCurrentAnimation)
		{
			ImGui::SetScrollHereY(0.5f);
		}
	}
	ImGui::EndChild();
	scrollToCurrentAnimation = false;

	ImGui::SetNextItemWidth(-1.0f);
	if (ImGui::InputTextWithHint("##BlendAnimationFilter", "Filter...", animationFilter, sizeof(animationFilter)))
	{
		scrollToCurrentAnimation = true;
	}
}

// ペアの比率を読む変数を選ぶ
void StudioBlendCanvas::DrawRateVariablePicker(BlendPairLink& pair)
{
	const auto& fields = Reflection::GetAllFields();

	// 表示するペアが変わったら、一覧を今の設定に合わせる
	if (rateInspectorPairId != pair.id)
	{
		rateInspectorPairId = pair.id;
		browseComponent = pair.componentName;
		scrollToCurrentField = true;
	}

	// 今の設定
	bool assigned = !pair.componentName.empty() && !pair.fieldName.empty();
	const FieldEntry* currentField = assigned ? Reflection::FindField(pair.componentName, pair.fieldName) : nullptr;
	if (!assigned)
	{
		ImGui::TextColored(errorTextColor, "No field selected.");
	}
	else if (!currentField)
	{
		ImGui::TextColored(errorTextColor, "%s.%s (missing)", pair.componentName.c_str(), pair.fieldName.c_str());
	}
	else
	{
		ImGui::Text("%s.%s (%s)", pair.componentName.c_str(), currentField->fieldLabel.c_str(), GetValueTypeName(currentField->type));
	}

	// 1段目：コンポーネント
	ImGui::Text("Component");
	ImGui::BeginChild("##RateComponents", ImVec2(0.0f, 120.0f), true);
	std::string previous;
	for (const auto& f : fields)
	{
		// 同じコンポーネントのフィールドは続けて並んでいるので、名前が変わったところで1行出す
		if (f.componentName == previous) continue;
		previous = f.componentName;

		bool open = (f.componentName == browseComponent);
		if (ImGui::Selectable(f.componentName.c_str(), open))
		{
			browseComponent = f.componentName;
		}
		if (open && scrollToCurrentField)
		{
			ImGui::SetScrollHereY(0.5f);
		}
	}
	ImGui::EndChild();

	// 2段目：フィールド
	ImGui::Text("Field");
	ImGui::BeginChild("##RateFields", ImVec2(0.0f, 160.0f), true);
	if (browseComponent.empty())
	{
		ImGui::TextDisabled("Select a component.");
	}
	for (const auto& f : fields)
	{
		if (f.componentName != browseComponent) continue;

		// 数として読めない型は、選べないようにする
		bool readable = (f.type != ValueType::Unsupported);
		bool current = (f.componentName == pair.componentName && f.fieldName == pair.fieldName);
		std::string label = f.fieldLabel + "  (" + GetValueTypeName(f.type) + ")";

		ImGui::PushID(f.fieldName.c_str());
		ImGuiSelectableFlags flags = readable ? ImGuiSelectableFlags_None : ImGuiSelectableFlags_Disabled;
		if (ImGui::Selectable(label.c_str(), current, flags) && !current)
		{
			pair.componentName = f.componentName;
			pair.fieldName = f.fieldName;
			pair.selectedComponent = FloatComponent::None; // 成分は選び直す
		}
		if (current && scrollToCurrentField)
		{
			ImGui::SetScrollHereY(0.5f);
		}
		ImGui::PopID();
	}
	ImGui::EndChild();
	scrollToCurrentField = false;

	// 3段目：成分(Float2〜4 のときだけ)
	if (currentField)
	{
		int count = Reflection::GetComponentCount(currentField->type);
		if (count > 0)
		{
			static const char* names[] = { "X", "Y", "Z", "W" };
			static const FloatComponent values[] = { FloatComponent::X, FloatComponent::Y, FloatComponent::Z, FloatComponent::W };

			ImGui::Text("Axis");
			for (int i = 0; i < count; ++i)
			{
				if (i > 0) ImGui::SameLine();
				if (ImGui::RadioButton(names[i], pair.selectedComponent == values[i]))
				{
					pair.selectedComponent = values[i];
				}
			}
			if (pair.selectedComponent == FloatComponent::None)
			{
				ImGui::TextColored(errorTextColor, "Select an axis.");
			}
		}
	}
}

// Output への線の一覧
void StudioBlendCanvas::DrawOutputLinkList()
{
	ImGui::Text("Output links (priority order)");

	// 入れ替えは描画が終わってから
	int moveId = -1;
	int moveOffset = 0;
	int number = 0;

	for (const auto& l : graph->outputLinks)
	{
		ImGui::PushID(l.id);

		if (ImGui::ArrowButton("##Up", ImGuiDir_Up)) { moveId = l.id; moveOffset = -1; }
		ImGui::SameLine();
		if (ImGui::ArrowButton("##Down", ImGuiDir_Down)) { moveId = l.id; moveOffset = +1; }
		ImGui::SameLine();

		// 条件の設定漏れは赤で表示
		bool invalid = (!l.always && l.condition.IsEmpty());
		if (invalid) ImGui::PushStyleColor(ImGuiCol_Text, errorTextColor);

		std::string label = std::to_string(++number) + ". " + GetFromName(l.fromId) + (l.always ? "  (Always)" : "");
		bool selected = (selectedKind == SelectionKind::OutputLink && selectedId == l.id);
		if (ImGui::Selectable(label.c_str(), selected))
		{
			ClearSelection();
			selectedKind = SelectionKind::OutputLink;
			selectedId = l.id;
		}

		if (invalid) ImGui::PopStyleColor();
		ImGui::PopID();
	}

	if (number == 0)
	{
		ImGui::TextColored(errorTextColor, "No links. Nothing will be played.");
	}

	if (moveId != -1)
	{
		graph->MoveOutputLink(moveId, moveOffset);
	}
}

// 材料の表示名
std::string StudioBlendCanvas::GetSourceName(int sourceId) const
{
	const BlendSourceNode* s = std::as_const(*graph).FindSource(sourceId);
	if (!s) return "(missing)";
	if (!s->name.empty()) return s->name;
	return s->clipName.empty() ? "(no clip)" : s->clipName;
}

// Output への線の元の表示名
std::string StudioBlendCanvas::GetFromName(int fromId) const
{
	switch (graph->GetNodeKind(fromId))
	{
	case BlendNodeKind::Source:
		return GetSourceName(fromId);
	case BlendNodeKind::PairLink:
	{
		const BlendPairLink* p = std::as_const(*graph).FindPairLink(fromId);
		return GetSourceName(p->fromId) + " + " + GetSourceName(p->toId);
	}
	default:
		return "(missing)";
	}
}

// 線の始点
bool StudioBlendCanvas::CalcLinkStart(int fromId, const ImVec2& toward, ImVec2& outStart) const
{
	// ペア：真ん中の点から
	auto midIt = pairMidpoints.find(fromId);
	if (midIt != pairMidpoints.end())
	{
		outStart = midIt->second;
		return true;
	}

	// 材料：縁から
	auto rectIt = nodeRects.find(fromId);
	if (rectIt != nodeRects.end())
	{
		outStart = StateGraphDraw::GetEdgePoint(rectIt->second.first, rectIt->second.second, toward);
		return true;
	}
	return false;
}