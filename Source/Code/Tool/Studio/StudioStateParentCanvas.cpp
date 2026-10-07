#include "StudioStateParentCanvas.h"
#include <cmath>
#include <cfloat>
#include <imgui.h>

void StudioStateParentCanvas::Initialize()
{
	// ノードエディタのコンテキストを作成
	canvas.Initialize();
}

// キャンバスを描く
void StudioStateParentCanvas::Draw(StateMachineData& graph)
{
	data = &graph;

	// キャンバス作成
	canvas.Begin("StateParentCanvas");

	// 削除や読み込みの後はノードエディタ側に残った選択を消す
	if (clearSelectionRequested)
	{
		ed::ClearSelection();
		clearSelectionRequested = false;
	}

	// ノードの矩形は毎フレーム作り直す
	nodeRects.clear();

	// ノード描画
	DrawEntryNode();
	DrawStateNodes();

	// 位置の反映は、読み込み直後の1フレームだけ
	needsApplyPositions = false; 

	// リンク描画
	DrawLinks();
	DrawPendingLink();

	// メニュー描画
	DrawBackgroundMenu();
	DrawNodeMenu();

	// 入力更新
	UpdateLinkCreation();
	UpdateSelection();

	// キャンバス終了
	canvas.End();
}

// 選択している物を削除
void StudioStateParentCanvas::DeleteSelection(StateMachineData& graph)
{
	// 選択中のリンクと遷移先のエントリーを削除
	if (selectedLinkId != -1)
	{
		graph.RemoveTransition(selectedLinkId);
		selectedLinkId = -1;
		return;
	}

	// 選択中のステート削除 (Entryは削除不可)
	for (int key : selectedNodeKeys)
	{
		if (key != StateMachineData::kEntryId)
		{
			graph.RemoveState(key);
		}
	}
	selectedNodeKeys.clear();
	clearSelectionRequested = true;
}

// 読み込み・新規作成のときに、選択などを戻す
void StudioStateParentCanvas::Reset()
{
	needsApplyPositions = true;
	selectedNodeKeys.clear();
	selectedLinkId = -1;
	pendingFromKey = -1;
	contextNodeKey = -1;
	clearSelectionRequested = true;
}

// インスペクター描画
void StudioStateParentCanvas::DrawInspector(StateMachineData& data)
{
	// 描画中だけ、受け取ったデータを指す
	this->data = &data;

	// リンク選択
	if (selectedLinkId != -1)
	{
		DrawLinkInspector(selectedLinkId);
		return;
	}

	// ノード複数選択
	if (selectedNodeKeys.size() > 1)
	{
		DrawMultiStateInspector();
		return;
	}

	// ノード1つ選択
	if (selectedNodeKeys.size() == 1)
	{
		int key = selectedNodeKeys[0];
		if (key == StateMachineData::kEntryId)
		{
			DrawEntryInspector();
			return;
		}
		if (StateNode* state = data.FindState(key))
		{
			DrawStateInspector(*state);
			return;
		}
	}

	// 未選択
	ImGui::TextDisabled("Nothing selected.");
}

// エントリーノード描画
void StudioStateParentCanvas::DrawEntryNode()
{
	ed::NodeId nodeId = ToNodeId(StateMachineData::kEntryId);
	
	// 読み込み直後はデータの位置にノードを反映
	if (needsApplyPositions)
	{
		ed::SetNodePosition(nodeId, ImVec2(data->entry.editorPos.x, data->entry.editorPos.y));
	}

	// Entryは緑で描画
	ed::PushStyleColor(ed::StyleColor_NodeBg, ImColor(46, 125, 50, 255));
	ed::PushStyleColor(ed::StyleColor_NodeBorder, ImColor(27, 94, 32, 255));
	ed::BeginNode(nodeId);
	ImGui::Text("Entry");
	ImGui::Dummy(ImVec2(80.0f, 0.0f));
	ed::EndNode();
	ed::PopStyleColor(2);

	// リンクの計算に使う矩形を記録
	nodeRects[StateMachineData::kEntryId] = { ImGui::GetItemRectMin(), ImGui::GetItemRectMax() };

	// ノードを動かしたら、データの位置に書き戻す
	ImVec2 pos = ed::GetNodePosition(nodeId);
	data->entry.editorPos = { pos.x, pos.y };
}

// ステート描画
void StudioStateParentCanvas::DrawStateNodes()
{
	for (auto& s : data->states)
	{
		ed::NodeId nodeId = ToNodeId(s.id);

		// 読み込み直後はデータの位置にノードを反映
		if (needsApplyPositions)
		{
			ed::SetNodePosition(nodeId, ImVec2(s.editorPos.x, s.editorPos.y));
		}

		// State毎の色を書く
		const float* c = s.editorColor;
		ed::PushStyleColor(ed::StyleColor_NodeBg, ImColor(c[0], c[1], c[2], 1.0f));
		ed::PushStyleColor(ed::StyleColor_NodeBorder, ImColor(c[0] * 0.6f, c[1] * 0.6f, c[2] * 0.6f, 1.0f));
		ed::BeginNode(nodeId);
		ImGui::Text("%s", s.name.c_str());
		ImGui::Dummy(ImVec2(80.0f, 0.0f));
		ed::EndNode();
		ed::PopStyleColor(2);

		// リンクの計算に使う矩形を記録
		nodeRects[s.id] = { ImGui::GetItemRectMin(), ImGui::GetItemRectMax() };

		// ノードを動かしたら、データの位置に書き戻す
		ImVec2 pos = ed::GetNodePosition(nodeId);
		s.editorPos = { pos.x, pos.y };
	}
}

// リンク描画
void StudioStateParentCanvas::DrawLinks()
{
	ImDrawList* drawList = ImGui::GetWindowDrawList();

	// クリック判定用の線は毎フレーム作り直す
	linkSegments.clear();

	// 2個のノードの間に複数リンクがあるとき何本目か記録する
	std::unordered_map<int64_t, int> laneCounts;

	// リンク1本分を描画
	auto drawOne = [&](int fromKey, const StateTransition& t)
		{
			// 遷移元と遷移先の組み合わせを1つの値にまとめて何本目かを数える
			int64_t pairKey = (static_cast<int64_t>(fromKey) << 32) | static_cast<uint32_t>(t.toStateId);
			int lane = laneCounts[pairKey]++;

			ImVec2 start, end;
			if (!CalcLinkPoints(fromKey, t.toStateId, lane, start, end)) return;

			bool selected = (t.id == selectedLinkId);
			bool invalid = (!t.always && t.condition.IsEmpty());

			// 正常(白色) 条件なし(赤色) 選択中(黄色)
			ImU32 color = IM_COL32(210, 210, 210, 255);
			if (invalid)  color = IM_COL32(230, 90, 90, 255);   // 条件の設定漏れ
			if (selected) color = IM_COL32(255, 200, 80, 255);  // 選択中

			DrawArrow(drawList, start, end, color, selected ? 3.0f : 2.0f);

			// クリック判定のために描いたリンクを記録
			linkSegments.push_back({ t.id, start, end });
		};

	// Entryから出るリンク
	for (const auto& t : data->entry.transitions) drawOne(StateMachineData::kEntryId, t);
	
	// 書くStateから出るリンク
	for (const auto& s : data->states)
	{
		for (const auto& t : s.transitions) drawOne(s.id, t);
	}
}

// 作成中のリンク描画
void StudioStateParentCanvas::DrawPendingLink()
{
	if (pendingFromKey == -1) return;

	auto it = nodeRects.find(pendingFromKey);
	if (it == nodeRects.end()) return;

	// 遷移元のノードの縁から、マウスの位置まで線を引く
	ImVec2 mouse = ImGui::GetMousePos();
	ImVec2 start = GetEdgePoint(it->second.first, it->second.second, mouse);
	DrawArrow(ImGui::GetWindowDrawList(), start, mouse, IM_COL32(255, 200, 80, 255), 2.0f);
}

// 背景右クリックのメニュー
void StudioStateParentCanvas::DrawBackgroundMenu()
{
	ed::Suspend();

	// ポップを開くリクエスト
	if (ed::ShowBackgroundContextMenu())
	{
		pendingCreatePos = ImGui::GetMousePos(); // マウス位置保存
		ImGui::OpenPopup("StateParentBackgroundMenu");
	}

	// ポップ内容
	if (ImGui::BeginPopup("StateParentBackgroundMenu"))
	{
		// 右クリックした位置にStateを作る
		if (ImGui::MenuItem("Create State"))
		{
			ImVec2 canvasPos = ed::ScreenToCanvas(pendingCreatePos);
			int newId = data->AddState("New State", { canvasPos.x, canvasPos.y });
			ed::SetNodePosition(ToNodeId(newId), canvasPos);
		}
		ImGui::EndPopup();
	}

	ed::Resume();
}

// ノード右クリックのメニュー
void StudioStateParentCanvas::DrawNodeMenu()
{
	ed::Suspend();

	ed::NodeId nodeId;
	if (ed::ShowNodeContextMenu(&nodeId))
	{
		// メニューを閉じるまでどのノードのメニューかを記録
		contextNodeKey = ToNodeKey(nodeId);

		// ポップを開くリクエスト
		ImGui::OpenPopup("StateParentNodeMenu");
	}

	// ポップ内容
	if (ImGui::BeginPopup("StateParentNodeMenu"))
	{
		// リンク作成開始
		if (ImGui::MenuItem("Make Link"))
		{
			pendingFromKey = contextNodeKey;
		}
		ImGui::EndPopup();
	}

	ed::Resume();
}

// リンク作成中の入力
void StudioStateParentCanvas::UpdateLinkCreation()
{
	if (pendingFromKey == -1) return;

	// Escで取り消し
	if (ImGui::IsKeyPressed(ImGuiKey_Escape))
	{
		pendingFromKey = -1;
		return;
	}

	// 
	if (!ImGui::IsMouseClicked(ImGuiMouseButton_Left)) return;

	// クリックした位置にノードがあればそこを行き先にする
	ed::NodeId hovered = ed::GetHoveredNode();
	if (hovered)
	{
		int toKey = ToNodeKey(hovered);

		// Entryへのリンクと、自分自身へのリンクは作らない
		if (toKey != StateMachineData::kEntryId && toKey != pendingFromKey)
		{
			int newId = data->AddTransition(pendingFromKey, toKey);
			if (newId != -1)
			{
				// 作ったリンクを選択状態にする
				selectedLinkId = newId;
				selectedNodeKeys.clear();
				ed::ClearSelection();
			}
		}
	}

	pendingFromKey = -1;
}

// 選択の更新
void StudioStateParentCanvas::UpdateSelection()
{
	// ノードの選択
	int count = ed::GetSelectedObjectCount();
	std::vector<ed::NodeId> nodeIds(count > 0 ? count : 1);
	int nodeCount = (count > 0) ? ed::GetSelectedNodes(nodeIds.data(), count) : 0;

	std::vector<int> keys;
	for (int i = 0; i < nodeCount; ++i)
	{
		int key = ToNodeKey(nodeIds[i]);
		// 削除した直後などで、データに無いノードが残っていても選択として扱わない
		if (IsExistingNode(key)) keys.push_back(key);
	}

	if (!keys.empty())
	{
		selectedNodeKeys = std::move(keys);
		selectedLinkId = -1;
	}
	else if (selectedLinkId == -1)
	{
		// ノードもリンクも選ばれていない
		selectedNodeKeys.clear();
	}

	// リンクの判定
	if (pendingFromKey != -1) return;
	if (!ed::IsBackgroundClicked()) return;

	// ノードエディタの中では、マウスの位置はキャンバスの座標で返る(線の座標と同じ)
	ImVec2 mouse = ImGui::GetMousePos();
	const float hitDistance = 6.0f;

	// クリックした位置から一番近いリンクを探す(遠すぎる場合は選ばない)
	int hitId = -1;
	float best = hitDistance;
	for (const auto& seg : linkSegments)
	{
		float d = DistanceToSegment(mouse, seg.start, seg.end);
		if (d < best)
		{
			best = d;
			hitId = seg.linkId;
		}
	}

	// 線のないところをクリックしたら選択解除
	selectedLinkId = hitId;
	if (hitId != -1)
	{
		// リンクを選んだら、ノードの選択は外す
		selectedNodeKeys.clear();
		ed::ClearSelection();
	}
}

// データに存在するノードか
bool StudioStateParentCanvas::IsExistingNode(int key) const
{
	if (key == StateMachineData::kEntryId) return true;
	return std::as_const(*data).FindState(key) != nullptr;
}

// ステートの情報
void StudioStateParentCanvas::DrawStateInspector(StateNode& state)
{
	ImGui::Text("State");
	ImGui::Separator();

	// 選ぶStateが変わったら、入力欄を今の名前で作り直す
	if (nameEditingStateId != state.id)
	{
		nameEditingStateId = state.id;
		strncpy_s(nameBuffer, state.name.c_str(), sizeof(nameBuffer) - 1);
		nameError.clear();
	}

	// 名前(Enterで確定。空や重複は確定しない)
	if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer), ImGuiInputTextFlags_EnterReturnsTrue))
	{
		if (data->RenameState(state.id, nameBuffer))
		{
			nameError.clear();
		}
		else
		{
			nameError = (nameBuffer[0] == '\0') ? "Name is empty." : "Name already used.";
		}
	}

	// 入力欄から離れたら、確定していない文字は今の名前に戻す
	if (ImGui::IsItemDeactivated() && state.name != nameBuffer)
	{
		strncpy_s(nameBuffer, state.name.c_str(), sizeof(nameBuffer) - 1);
	}
	if (!nameError.empty())
	{
		ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", nameError.c_str());
	}

	// 色
	ImGui::ColorEdit3("Color", state.editorColor);
	ImGui::Spacing();

	// 出ていくリンクの一覧
	DrawLinkList(state.id, state.transitions);
	ImGui::Spacing();

	// 子グラフへ移動
	ImGui::BeginDisabled();
	ImGui::Button("Open Child Graph", ImVec2(-1.0f, 0.0f));
	ImGui::EndDisabled();

	// 削除
	if (DrawDeleteButton("Delete State"))
	{
		DeleteSelection(*data);
	}
}

// エントリーの設定
void StudioStateParentCanvas::DrawEntryInspector()
{
	ImGui::Text("Entry");
	ImGui::Separator();

	// 出ていくリンクの一覧
	DrawLinkList(StateMachineData::kEntryId, data->entry.transitions);
}

// 複数ステートの選択時
void StudioStateParentCanvas::DrawMultiStateInspector()
{
	ImGui::Text("%d States selected", static_cast<int>(selectedNodeKeys.size()));
	ImGui::Separator();

	// まとめて削除
	if (DrawDeleteButton("Delete States"))
	{
		DeleteSelection(*data);
	}
}

// リンクの設定
void StudioStateParentCanvas::DrawLinkInspector(int linkId)
{
	StateTransition* link = data->FindTransition(linkId);
	int fromKey = -1, index = -1, count = 0;
	if (!link || !FindLinkOwner(linkId, fromKey, index, count))
	{
		ImGui::TextDisabled("Nothing selected.");
		return;
	}

	ImGui::Text("Link");
	ImGui::Separator();

	// 遷移元 → 遷移先
	ImGui::Text("%s  ->  %s", GetNodeName(fromKey).c_str(), GetNodeName(link->toStateId).c_str());

	// 優先度(遷移元の中で何番目か)
	ImGui::Text("Priority: %d / %d", index + 1, count);
	ImGui::SameLine();
	if (ImGui::ArrowButton("##LinkUp", ImGuiDir_Up))     data->MoveTransition(linkId, -1);
	ImGui::SameLine();
	if (ImGui::ArrowButton("##LinkDown", ImGuiDir_Down)) data->MoveTransition(linkId, +1);
	ImGui::Spacing();

	// 常に成立
	ImGui::Checkbox("Always", &link->always);

	// ブレンド
	ImGui::Checkbox("Blend", &link->blend.enabled);
	ImGui::BeginDisabled(!link->blend.enabled);
	ImGui::DragFloat("Blend Duration", &link->blend.duration, 0.01f, 0.0f, 5.0f, "%.2f s");
	ImGui::EndDisabled();
	ImGui::Spacing();

	// 条件の状態
	if (link->always)
	{
		ImGui::TextDisabled("Condition: Always");
	}
	else if (link->condition.IsEmpty())
	{
		ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Condition: Empty");
	}
	else
	{
		ImGui::Text("Condition: Set");
	}

	// 条件の編集へ
	ImGui::BeginDisabled();
	ImGui::Button("Edit Condition", ImVec2(-1.0f, 0.0f));
	ImGui::EndDisabled();

	// 削除
	if (DrawDeleteButton("Delete Link"))
	{
		DeleteSelection(*data);
	}
}

// 出ていくリンクの一覧
void StudioStateParentCanvas::DrawLinkList(int fromKey, std::vector<StateTransition>& links)
{
	ImGui::Text("Links (priority order)");

	if (links.empty())
	{
		ImGui::TextDisabled("No links.");
		return;
	}

	// 入れ替えは描画が終わってから
	int moveId = -1;
	int moveOffset = 0;

	for (int i = 0; i < static_cast<int>(links.size()); ++i)
	{
		const StateTransition& t = links[i];
		ImGui::PushID(t.id);

		// 上下で優先度を入れ替える
		if (ImGui::ArrowButton("##Up", ImGuiDir_Up)) { moveId = t.id; moveOffset = -1; }
		ImGui::SameLine();
		if (ImGui::ArrowButton("##Down", ImGuiDir_Down)) { moveId = t.id; moveOffset = +1; }
		ImGui::SameLine();

		// 条件の設定漏れは赤で表示
		bool invalid = (!t.always && t.condition.IsEmpty());
		if (invalid) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));

		// クリックでそのリンクを選ぶ
		std::string label = std::to_string(i + 1) + ". -> " + GetNodeName(t.toStateId);
		if (ImGui::Selectable(label.c_str(), t.id == selectedLinkId))
		{
			selectedLinkId = t.id;
			selectedNodeKeys.clear();
			clearSelectionRequested = true; // キャンバス側のノードの選択も外す
		}

		if (invalid) ImGui::PopStyleColor();
		ImGui::PopID();
	}

	if (moveId != -1)
	{
		data->MoveTransition(moveId, moveOffset);
	}
	(void)fromKey;
}

// 削除ボタン
bool StudioStateParentCanvas::DrawDeleteButton(const char* label)
{
	ImGui::Spacing();
	ImGui::Spacing();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.15f, 0.15f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.2f, 0.2f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));
	bool pressed = ImGui::Button(label, ImVec2(-1.0f, 0.0f));
	ImGui::PopStyleColor(3);

	return pressed;
}

// リンクの遷移元と、遷移元の中での何番目かを探す
bool StudioStateParentCanvas::FindLinkOwner(int linkId, int& outFromKey, int& outIndex, int& outCount) const
{
	auto search = [&](int fromKey, const std::vector<StateTransition>& list)
		{
			for (int i = 0; i < static_cast<int>(list.size()); ++i)
			{
				if (list[i].id == linkId)
				{
					outFromKey = fromKey;
					outIndex = i;
					outCount = static_cast<int>(list.size());
					return true;
				}
			}
			return false;
		};

	if (search(StateMachineData::kEntryId, data->entry.transitions)) return true;
	for (const auto& s : data->states)
	{
		if (search(s.id, s.transitions)) return true;
	}
	return false;
}

// ノードのキーから、表示用の名前を作る
std::string StudioStateParentCanvas::GetNodeName(int key) const
{
	if (key == StateMachineData::kEntryId) return "Entry";
	if (const StateNode* s = std::as_const(*data).FindState(key)) return s->name;
	return "(missing)";
}

// fromKeyからtoStateIdへのリンクがあるか
bool StudioStateParentCanvas::HasLink(int fromKey, int toStateId) const
{
	// 遷移元のリンクの一覧
	const std::vector<StateTransition>* list = nullptr;
	if (fromKey == StateMachineData::kEntryId)
	{
		list = &data->entry.transitions;
	}
	else if (const StateNode* s = std::as_const(*data).FindState(fromKey))
	{
		list = &s->transitions;
	}
	if (!list) return false;

	for (const auto& t : *list)
	{
		if (t.toStateId == toStateId) return true;
	}
	return false;
}

// 線の始点と終点を求める
bool StudioStateParentCanvas::CalcLinkPoints(int fromKey, int toStateId, int lane, ImVec2& outStart, ImVec2& outEnd) const
{
	auto fromIt = nodeRects.find(fromKey);
	auto toIt = nodeRects.find(toStateId);
	if (fromIt == nodeRects.end() || toIt == nodeRects.end()) return false;

	const auto& [fromMin, fromMax] = fromIt->second;
	const auto& [toMin, toMax] = toIt->second;
	ImVec2 fromCenter((fromMin.x + fromMax.x) * 0.5f, (fromMin.y + fromMax.y) * 0.5f);
	ImVec2 toCenter((toMin.x + toMax.x) * 0.5f, (toMin.y + toMax.y) * 0.5f);

	// ノードの中心同士を結ぶ線がそれぞれの縁と交わる点を始点と終点にする
	outStart = GetEdgePoint(fromMin, fromMax, toCenter);
	outEnd = GetEdgePoint(toMin, toMax, fromCenter);

	// 逆向きの線がある場合や、同じ向きに複数ある場合は、線を横にずらす
	bool hasReverse = HasLink(toStateId, fromKey);
	float offset = (hasReverse ? 8.0f : 0.0f) + lane * 10.0f;
	if (offset <= 0.0f) return true;

	// 往復の線が反対側にずれるように、IDの小さいほうから見た向きを基準にする
	bool fromIsSmaller = fromKey < toStateId;
	ImVec2 baseFrom = fromIsSmaller ? fromCenter : toCenter;
	ImVec2 baseTo = fromIsSmaller ? toCenter : fromCenter;
	ImVec2 dir(baseTo.x - baseFrom.x, baseTo.y - baseFrom.y);
	float len = sqrtf(dir.x * dir.x + dir.y * dir.y);
	if (len < 0.0001f) return true;

	// 基準の向きに垂直な方向へずらす
	float sign = (hasReverse && !fromIsSmaller) ? -1.0f : 1.0f;
	ImVec2 perp(-dir.y / len * offset * sign, dir.x / len * offset * sign);
	outStart = ImVec2(outStart.x + perp.x, outStart.y + perp.y);
	outEnd = ImVec2(outEnd.x + perp.x, outEnd.y + perp.y);
	return true;
}

// 目的地へ伸ばすリンクが矩形の縁と交わる点
ImVec2 StudioStateParentCanvas::GetEdgePoint(const ImVec2& rectMin, const ImVec2& rectMax, const ImVec2& target)
{
	ImVec2 center((rectMin.x + rectMax.x) * 0.5f, (rectMin.y + rectMax.y) * 0.5f);
	float halfW = (rectMax.x - rectMin.x) * 0.5f;
	float halfH = (rectMax.y - rectMin.y) * 0.5f;

	ImVec2 dir(target.x - center.x, target.y - center.y);
	if (dir.x == 0.0f && dir.y == 0.0f) return center;

	// 横の辺と縦の辺のうち、先にぶつかるほうの倍率を使う
	float scaleX = (dir.x != 0.0f) ? halfW / fabsf(dir.x) : FLT_MAX;
	float scaleY = (dir.y != 0.0f) ? halfH / fabsf(dir.y) : FLT_MAX;
	float scale = (scaleX < scaleY) ? scaleX : scaleY;
	return ImVec2(center.x + dir.x * scale, center.y + dir.y * scale);
}

// 矢印描画
void StudioStateParentCanvas::DrawArrow(ImDrawList* drawList, const ImVec2& start, const ImVec2& end, ImU32 color, float thickness)
{
	ImVec2 dir(end.x - start.x, end.y - start.y);
	float len = sqrtf(dir.x * dir.x + dir.y * dir.y);
	if (len < 0.0001f) return;
	dir.x /= len;
	dir.y /= len;

	const float arrowLength = 14.0f;
	const float arrowWidth = 11.0f;

	// 線は矢印の根元まで
	ImVec2 base(end.x - dir.x * arrowLength, end.y - dir.y * arrowLength);
	drawList->AddLine(start, base, color, thickness);

	// 矢印の三角形
	ImVec2 perp(-dir.y, dir.x);
	ImVec2 left(base.x + perp.x * arrowWidth * 0.5f, base.y + perp.y * arrowWidth * 0.5f);
	ImVec2 right(base.x - perp.x * arrowWidth * 0.5f, base.y - perp.y * arrowWidth * 0.5f);
	drawList->AddTriangleFilled(end, left, right, color);
}

// 点と線分の距離
float StudioStateParentCanvas::DistanceToSegment(const ImVec2& p, const ImVec2& a, const ImVec2& b)
{
	ImVec2 ab(b.x - a.x, b.y - a.y);
	ImVec2 ap(p.x - a.x, p.y - a.y);
	float lenSq = ab.x * ab.x + ab.y * ab.y;
	
	// 点pを線分abに投影した位置。線分の外に出たら端に寄せる
	float t = (lenSq > 0.0f) ? (ap.x * ab.x + ap.y * ab.y) / lenSq : 0.0f;
	t = (t < 0.0f) ? 0.0f : (t > 1.0f ? 1.0f : t);
	
	// 線分上で一番近い点との距離
	ImVec2 closest(a.x + ab.x * t, a.y + ab.y * t);
	float dx = p.x - closest.x;
	float dy = p.y - closest.y;
	return sqrtf(dx * dx + dy * dy);
}

// ノードのキー → ノードエディタのID
ed::NodeId StudioStateParentCanvas::ToNodeId(int key)
{
	if (key == StateMachineData::kEntryId) return ed::NodeId(static_cast<uint64_t>(kEntryNodeId));
	return ed::NodeId(static_cast<uint64_t>(key));
}

// ノードエディタのID → ノードのキー
int StudioStateParentCanvas::ToNodeKey(ed::NodeId nodeId)
{
	if (nodeId.Get() == static_cast<uint64_t>(kEntryNodeId)) return StateMachineData::kEntryId;
	return static_cast<int>(nodeId.Get());
}