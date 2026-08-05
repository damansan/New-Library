#include "StudioSequencer.h"
#include "ECS/Components/SkinnedMeshComponent.h"
#include "ECS/Components/AnimatorComponent.h"
#include <algorithm>

// ImGui描画
void StudioSequencer::DrawGUI(entt::registry& registry, StudioContext& context)
{
	if (!context.showSequencer) return;

	std::string panelName = "Sequencer##" + std::to_string(windowId);
	ImGui::Begin(panelName.c_str());

	RefreshAvailableAnimations(registry);

	auto view = registry.view<SkinnedMeshComponent, AnimatorComponent>();
	if (view.begin() == view.end())
	{
		ImGui::Text("No skinned model.");
		ImGui::End();
		return;
	}

	auto entity = *view.begin();
	auto& skinned = view.get<SkinnedMeshComponent>(entity);
	auto& animator = view.get<AnimatorComponent>(entity);

	if (!skinned.model)
	{
		ImGui::End();
		return;
	}

	if (animator.currentAnimation.empty())
	{
		ImGui::TextDisabled("Select an animation from the Animator panel.");
		ImGui::End();
		return;
	}

	// 選んでいるクリップが切り替わったら、選択状態をリセットする
	if (selectedClipName != animator.currentAnimation)
	{
		selectedClipName = animator.currentAnimation;
		selectedType = SelectionType::None;
		selectedId = -1;
	}

	// このクリップの長さを取得する
	float clipLength = 0.0f;
	for (const auto& anim : skinned.model->GetAnimations())
	{
		if (anim.name == selectedClipName)
		{
			clipLength = anim.secondsLength;
			break;
		}
	}

	// このクリップが今再生中なら、プレビュー時刻として使う
	float previewTime = (animator.currentAnimation == selectedClipName) ? animator.animationTime : 0.0f;

	SequencerClipData* clip = sequencerData.FindOrCreateClip(selectedClipName);

	const float detailPanelWidth = 220.0f;

	ImGui::BeginChild("SequencerTracks", ImVec2(-detailPanelWidth, 0.0f));
	DrawPointTrack(*clip, previewTime, clipLength);
	DrawRangeTrack(*clip, previewTime, clipLength);
	ImGui::EndChild();

	ImGui::SameLine();

	ImGui::BeginChild("SequencerDetail", ImVec2(detailPanelWidth, 0.0f), true);
	DrawDetailPanel(*clip);
	ImGui::EndChild();

	ImGui::End();
}

// ツールバー
void StudioSequencer::DrawToolbar(SequencerClipData& clip, float previewTime)
{
	ImGui::TextDisabled("Clip: %s", clip.clipName.c_str());
	ImGui::TextDisabled("Ranges: %d  Points: %d", (int)clip.ranges.size(), (int)clip.points.size());
	ImGui::Separator();
}
// Pointのトラック描画
void StudioSequencer::DrawPointTrack(SequencerClipData& clip, float previewTime, float clipLength)
{
	ImGui::Text("Points");
	ImGui::SameLine(ImGui::GetContentRegionAvail().x - 60.0f);
	if (ImGui::Button("+ Add##Point"))
	{
		SequencerPoint p;
		p.id = clip.GenerateId();
		p.time = TimeToNormalized(previewTime, clipLength);
		clip.points.push_back(p);

		selectedType = SelectionType::Point;
		selectedId = p.id;
	}

	ImVec2 trackMin = ImGui::GetCursorScreenPos();
	float trackWidth = ImGui::GetContentRegionAvail().x;
	const float trackHeight = 24.0f;
	ImVec2 trackMax(trackMin.x + trackWidth, trackMin.y + trackHeight);

	ImDrawList* dl = ImGui::GetWindowDrawList();
	dl->AddRectFilled(trackMin, trackMax, IM_COL32(40, 40, 40, 255));

	auto timeToX = [&](float normalized) { return trackMin.x + normalized * trackWidth; };

	// プレイヘッド
	float phX = timeToX(TimeToNormalized(previewTime, clipLength));
	dl->AddLine(ImVec2(phX, trackMin.y), ImVec2(phX, trackMax.y), IM_COL32(29, 158, 117, 200), 1.5f);

	ImGui::PushID("PointTrack");
	for (auto& p : clip.points)
	{
		float x = timeToX(p.time);
		ImVec2 markerMin(x - 3.0f, trackMin.y);
		ImVec2 markerMax(x + 3.0f, trackMax.y);

		bool isSelected = (selectedType == SelectionType::Point && selectedId == p.id);
		ImU32 color = isSelected ? IM_COL32(255, 200, 80, 255) : IM_COL32(120, 170, 255, 255);
		dl->AddRectFilled(markerMin, markerMax, color);

		ImGui::SetCursorScreenPos(markerMin);
		ImGui::PushID(p.id);
		ImGui::InvisibleButton("marker", ImVec2(6.0f, trackHeight));

		if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
		{
			float mouseX = ImGui::GetMousePos().x;
			float normalized = (mouseX - trackMin.x) / trackWidth;
			p.time = std::clamp(normalized, 0.0f, 1.0f);
		}

		if (ImGui::IsItemClicked())
		{
			selectedType = SelectionType::Point;
			selectedId = p.id;
		}

		ImGui::PopID();
	}
	ImGui::PopID();

	ImGui::SetCursorScreenPos(ImVec2(trackMin.x, trackMax.y + 4.0f));
	ImGui::Dummy(ImVec2(0.0f, 0.0f));
}
// Rangeのトラック描画
void StudioSequencer::DrawRangeTrack(SequencerClipData& clip, float previewTime, float clipLength)
{
	ImGui::Spacing();
	ImGui::Text("Ranges");
	ImGui::SameLine(ImGui::GetContentRegionAvail().x - 60.0f);
	if (ImGui::Button("+ Add##Range"))
	{
		SequencerRange r;
		r.id = clip.GenerateId();
		r.start = TimeToNormalized(previewTime, clipLength);
		r.end = (std::min)(1.0f, r.start + 0.1f);
		clip.ranges.push_back(r);

		selectedType = SelectionType::Range;
		selectedId = r.id;
	}

	float trackWidth = ImGui::GetContentRegionAvail().x;
	const float rowHeight = 24.0f;
	const float rowSpacing = 4.0f;

	auto timeToX = [&](float normalized, float originX) { return originX + normalized * trackWidth; };

	ImDrawList* dl = ImGui::GetWindowDrawList();

	ImGui::PushID("RangeTrack");
	for (auto& r : clip.ranges)
	{
		ImVec2 rowMin = ImGui::GetCursorScreenPos();
		ImVec2 rowMax(rowMin.x + trackWidth, rowMin.y + rowHeight);

		dl->AddRectFilled(rowMin, rowMax, IM_COL32(40, 40, 40, 255));

		float startX = timeToX(r.start, rowMin.x);
		float endX = timeToX(r.end, rowMin.x);

		bool isSelected = (selectedType == SelectionType::Range && selectedId == r.id);
		ImU32 barColor = isSelected ? IM_COL32(255, 200, 80, 180) : IM_COL32(120, 170, 255, 180);
		ImU32 handleColor = isSelected ? IM_COL32(255, 200, 80, 255) : IM_COL32(120, 170, 255, 255);

		dl->AddRectFilled(ImVec2(startX, rowMin.y), ImVec2(endX, rowMax.y), barColor);
		dl->AddRectFilled(ImVec2(startX - 3.0f, rowMin.y), ImVec2(startX + 3.0f, rowMax.y), handleColor);
		dl->AddRectFilled(ImVec2(endX - 3.0f, rowMin.y), ImVec2(endX + 3.0f, rowMax.y), handleColor);
		dl->AddText(ImVec2(startX + 6.0f, rowMin.y + 4.0f), IM_COL32(255, 255, 255, 255), GetSequencerFlagName(r.flag));

		// プレイヘッド
		float phX = timeToX(TimeToNormalized(previewTime, clipLength), rowMin.x);
		dl->AddLine(ImVec2(phX, rowMin.y), ImVec2(phX, rowMax.y), IM_COL32(29, 158, 117, 200), 1.5f);

		ImGui::PushID(r.id);

		// 中央(バー全体)
		ImGui::SetCursorScreenPos(ImVec2(startX + 6.0f, rowMin.y));
		ImGui::InvisibleButton("body", ImVec2((std::max)(1.0f, endX - startX - 12.0f), rowHeight));
		if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
		{
			float deltaNorm = ImGui::GetIO().MouseDelta.x / trackWidth;
			float width = r.end - r.start;
			r.start = std::clamp(r.start + deltaNorm, 0.0f, 1.0f - width);
			r.end = r.start + width;
		}
		if (ImGui::IsItemClicked())
		{
			selectedType = SelectionType::Range;
			selectedId = r.id;
		}

		// 左端(開始位置)
		ImGui::SetCursorScreenPos(ImVec2(startX - 4.0f, rowMin.y));
		ImGui::InvisibleButton("startHandle", ImVec2(8.0f, rowHeight));
		if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
		{
			float mouseX = ImGui::GetMousePos().x;
			float normalized = (mouseX - rowMin.x) / trackWidth;
			r.start = std::clamp(normalized, 0.0f, r.end);
		}

		// 右端(終了位置)
		ImGui::SetCursorScreenPos(ImVec2(endX - 4.0f, rowMin.y));
		ImGui::InvisibleButton("endHandle", ImVec2(8.0f, rowHeight));
		if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
		{
			float mouseX = ImGui::GetMousePos().x;
			float normalized = (mouseX - rowMin.x) / trackWidth;
			r.end = std::clamp(normalized, r.start, 1.0f);
		}

		ImGui::PopID();

		ImGui::SetCursorScreenPos(ImVec2(rowMin.x, rowMax.y + rowSpacing));
		ImGui::Dummy(ImVec2(0.0f, 0.0f));
	}
	ImGui::PopID();
}
// 詳細パネル
void StudioSequencer::DrawDetailPanel(SequencerClipData& clip)
{
	if (selectedType == SelectionType::Range)
	{
		SequencerRange* range = nullptr;
		for (auto& r : clip.ranges)
		{
			if (r.id == selectedId) { range = &r; break; }
		}

		if (!range)
		{
			selectedType = SelectionType::None;
			selectedId = -1;
			ImGui::TextDisabled("No selection.");
			return;
		}

		ImGui::Text("Range");
		ImGui::Separator();

		if (ImGui::BeginCombo("Flag", GetSequencerFlagName(range->flag)))
		{
			for (int i = 0; i <= (int)SequencerFlag::Attack; i++)
			{
				SequencerFlag f = (SequencerFlag)i;
				bool isSelected = (range->flag == f);
				if (ImGui::Selectable(GetSequencerFlagName(f), isSelected))
				{
					range->flag = f;
				}
			}
			ImGui::EndCombo();
		}

		float startPct = range->start * 100.0f;
		float endPct = range->end * 100.0f;

		if (ImGui::DragFloat("Start %", &startPct, 0.1f, 0.0f, endPct))
		{
			range->start = startPct * 0.01f;
		}
		if (ImGui::DragFloat("End %", &endPct, 0.1f, startPct, 100.0f))
		{
			range->end = endPct * 0.01f;
		}

		int rangeId = range->id;
		DrawDeleteButton("Delete Range", [this, &clip, rangeId]() { DeleteRange(clip, rangeId); });
		return;
	}

	if (selectedType == SelectionType::Point)
	{
		SequencerPoint* point = nullptr;
		for (auto& p : clip.points)
		{
			if (p.id == selectedId) { point = &p; break; }
		}

		if (!point)
		{
			selectedType = SelectionType::None;
			selectedId = -1;
			ImGui::TextDisabled("No selection.");
			return;
		}

		ImGui::Text("Point");
		ImGui::Separator();

		if (ImGui::BeginCombo("Flag", GetSequencerFlagName(point->flag)))
		{
			for (int i = 0; i <= (int)SequencerFlag::Attack; i++)
			{
				SequencerFlag f = (SequencerFlag)i;
				bool isSelected = (point->flag == f);
				if (ImGui::Selectable(GetSequencerFlagName(f), isSelected))
				{
					point->flag = f;
				}
			}
			ImGui::EndCombo();
		}

		float timePct = point->time * 100.0f;
		if (ImGui::DragFloat("Time %", &timePct, 0.1f, 0.0f, 100.0f))
		{
			point->time = timePct * 0.01f;
		}

		int pointId = point->id;
		DrawDeleteButton("Delete Point", [this, &clip, pointId]() { DeletePoint(clip, pointId); });
		return;
	}

	ImGui::TextDisabled("No selection.");
}

// Range削除
void StudioSequencer::DeleteRange(SequencerClipData& clip, int id)
{
	clip.ranges.erase(
		std::remove_if(clip.ranges.begin(), clip.ranges.end(),
			[id](const SequencerRange& r) { return r.id == id; }),
		clip.ranges.end()
	);
	selectedType = SelectionType::None;
	selectedId = -1;
}
// Point削除
void StudioSequencer::DeletePoint(SequencerClipData& clip, int id)
{
	clip.points.erase(
		std::remove_if(clip.points.begin(), clip.points.end(),
			[id](const SequencerPoint& p) { return p.id == id; }),
		clip.points.end()
	);
	selectedType = SelectionType::None;
	selectedId = -1;
}

// 削除ボタン
void StudioSequencer::DrawDeleteButton(const char* label, const std::function<void()>& onDelete)
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

// 秒数を0.0～1.0に
float StudioSequencer::TimeToNormalized(float time, float clipLength)
{
	if (clipLength <= 0.0001f) return 0.0f;
	return time / clipLength;
}
// 0.0～1.0を秒数に
float StudioSequencer::NormalizedToTime(float normalized, float clipLength)
{
	return normalized * clipLength;
}

// アニメーション一覧を更新する
void StudioSequencer::RefreshAvailableAnimations(entt::registry& registry)
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
// シーケンサーフラグの表示名を返す
const char* StudioSequencer::GetSequencerFlagName(SequencerFlag flag)
{
	switch (flag)
	{
	case SequencerFlag::AnimExit: return "AnimExit";
	case SequencerFlag::Attack:   return "Attack";
	}
	return "?";
}
