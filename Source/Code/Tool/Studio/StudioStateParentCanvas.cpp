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
	if (selectedNodeKey != -1 && selectedNodeKey != StateMachineData::kEntryId)
	{
		graph.RemoveState(selectedNodeKey);
		selectedNodeKey = -1;
	}
}

// 読み込み・新規作成のときに、選択などを戻す
void StudioStateParentCanvas::Reset()
{
	needsApplyPositions = true;
	selectedNodeKey = -1;
	selectedLinkId = -1;
	pendingFromKey = -1;
	contextNodeKey = -1;
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

		// Entryは削除できない
		if (contextNodeKey != StateMachineData::kEntryId && ImGui::MenuItem("Delete"))
		{
			data->RemoveState(contextNodeKey);
			if (selectedNodeKey == contextNodeKey) selectedNodeKey = -1;
			selectedLinkId = -1;
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
				selectedNodeKey = -1;
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
	ed::NodeId selectedNode;
	if (ed::GetSelectedNodes(&selectedNode, 1) > 0)
	{
		selectedNodeKey = ToNodeKey(selectedNode);
		selectedLinkId = -1;
	}
	else if (selectedLinkId == -1)
	{
		// ノードもリンクも選ばれていない
		selectedNodeKey = -1;
	}

	// リンクの判定
	if (pendingFromKey != -1) return;
	if (!ImGui::IsMouseClicked(ImGuiMouseButton_Left)) return;
	if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows)) return;
	if (ed::GetHoveredNode()) return;

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
		selectedNodeKey = -1;
		ed::ClearSelection();
	}
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