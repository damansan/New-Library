#include "StudioTimelineWidget.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace
{
	const ImU32 RulerBgColor = IM_COL32(40, 42, 46, 255);
	const ImU32 TickColor = IM_COL32(120, 120, 120, 255);
	const ImU32 LabelColor = IM_COL32(200, 200, 200, 255);
	const ImU32 PinColor = IM_COL32(255, 82, 82, 255);
	const ImU32 RangeHandleColor = IM_COL32(255, 255, 255, 255);
	const ImU32 RangeHandleActiveColor = IM_COL32(255, 213, 79, 255);
}

// 目盛りを描いて、ピンのドラッグを処理する
bool StudioTimelineWidget::Begin(const char* id, float clipLength, float frameStep, float& inOutTime)
{
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	const ImVec2 cursor = ImGui::GetCursorScreenPos();
	const float availWidth = ImGui::GetContentRegionAvail().x;

	left = cursor.x + LeftPadding;
	width = std::max(availWidth - LeftPadding - RightPadding, 1.0f);
	top = cursor.y;
	length = std::max(clipLength, 0.0001f);

	// 目盛りの領域をつかめるようにする
	ImGui::InvisibleButton(id, ImVec2(availWidth, PinHeadHeight + RulerHeight));

	bool changed = false;
	if (ImGui::IsItemActive())
	{
		const float t = SnapToFrame(XToTime(ImGui::GetIO().MousePos.x), frameStep, length);
		if (t != inOutTime)
		{
			inOutTime = t;
			changed = true;
		}
	}
	time = inOutTime;

	DrawRuler(drawList, frameStep);
	return changed;
}

// トラックを描き終えたら呼ぶ
void StudioTimelineWidget::End()
{
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	const float bottom = ImGui::GetCursorScreenPos().y;
	const float x = TimeToX(time);

	// 細い線：頭の下から、トラックの下端まで
	drawList->AddLine(ImVec2(x, top + PinHeadHeight), ImVec2(x, bottom), PinColor, 1.0f);

	// 頭は最後に描いて、一番手前に出す
	DrawPinHead(drawList);
}
// 再生ボタンと現在時刻
bool StudioTimelineWidget::DrawToolbar(bool& isPlaying, float time, float frameStep)
{
	bool changed = false;

	if (ImGui::Button(isPlaying ? u8"■" : u8"▶", ImVec2(28.0f, 0.0f)))
	{
		isPlaying = !isPlaying;
		changed = true;
	}

	ImGui::SameLine();
	const int frame = (frameStep > 0.0f) ? (int)std::round(time / frameStep) : 0;
	ImGui::Text("%.3f s / F%d", time, frame);

	return changed;
}

// フレームに吸い付かせる
float StudioTimelineWidget::SnapToFrame(float t, float frameStep, float clipLength)
{
	if (frameStep <= 0.0f) return std::clamp(t, 0.0f, clipLength);
	return std::clamp(std::round(t / frameStep) * frameStep, 0.0f, clipLength);
}

// 範囲の端のピン
bool StudioTimelineWidget::DrawRangeHandles(const char* id, float handleY, float frameStep, float minLength, float& inOutStart, float& inOutEnd)
{
	ImDrawList* drawList = ImGui::GetWindowDrawList();

	// 頭の位置につかめる領域を置くため、カーソルを一時的に動かす
	const ImVec2 savedCursor = ImGui::GetCursorScreenPos();
	bool changed = false;

	ImGui::PushID(id);
	for (int i = 0; i < 2; i++)
	{
		const bool isStart = (i == 0);
		float& value = isStart ? inOutStart : inOutEnd;

		ImGui::PushID(i);
		ImGui::SetCursorScreenPos(ImVec2(TimeToX(value) - RangeHandleHalfWidth, handleY));
		ImGui::InvisibleButton("##RangeHandle", ImVec2(RangeHandleHalfWidth * 2.0f, RangeHandleHeight));
		const bool active = ImGui::IsItemActive();
		const bool hovered = ImGui::IsItemHovered();
		ImGui::PopID();

		if (active)
		{
			float t = SnapToFrame(XToTime(ImGui::GetIO().MousePos.x), frameStep, length);

			// もう一方の端と、最低 minLength は空ける
			if (isStart) t = std::min(t, inOutEnd - minLength);
			else         t = std::max(t, inOutStart + minLength);
			t = std::clamp(t, 0.0f, length);

			if (t != value)
			{
				value = t;
				changed = true;
			}
		}

		DrawRangeHandle(drawList, TimeToX(value), handleY, active || hovered);
	}
	ImGui::PopID();

	ImGui::SetCursorScreenPos(savedCursor);
	return changed;
}

// 時刻とX座標の変換
float StudioTimelineWidget::TimeToX(float t) const
{
	return left + (t / length) * width;
}
float StudioTimelineWidget::XToTime(float x) const
{
	return std::clamp((x - left) / width * length, 0.0f, length);
}


// 目盛り
void StudioTimelineWidget::DrawRuler(ImDrawList* drawList, float frameStep) const
{
	const float rulerTop = top + PinHeadHeight;
	const float bottom = rulerTop + RulerHeight;
	drawList->AddRectFilled(
		ImVec2(left - LeftPadding, rulerTop),
		ImVec2(left + width + RightPadding, bottom),
		RulerBgColor);

	// フレームごとの細かい目盛り
	if (frameStep > 0.0f && width * frameStep / length >= 4.0f)
	{
		const int frameCount = (int)std::round(length / frameStep);
		for (int i = 0; i <= frameCount; i++)
		{
			const float x = TimeToX(i * frameStep);
			drawList->AddLine(ImVec2(x, bottom - 4.0f), ImVec2(x, bottom), TickColor);
		}
	}

	// 秒の目盛り
	const float candidates[] = { 0.1f, 0.2f, 0.5f, 1.0f, 2.0f, 5.0f, 10.0f };
	float major = candidates[6];
	for (float c : candidates)
	{
		if (width * c / length >= 60.0f)
		{
			major = c;
			break;
		}
	}

	const int majorCount = (int)std::floor(length / major + 0.0001f);
	for (int i = 0; i <= majorCount; i++)
	{
		const float t = i * major;
		const float x = TimeToX(t);
		drawList->AddLine(ImVec2(x, bottom - 12.0f), ImVec2(x, bottom), TickColor);

		char label[16];
		snprintf(label, sizeof(label), "%.1f", t);
		drawList->AddText(ImVec2(x + 3.0f, rulerTop + 2.0f), LabelColor, label);
	}
}

// ピンの頭
void StudioTimelineWidget::DrawPinHead(ImDrawList* drawList) const
{
	const float x = TimeToX(time);
	const float radius = 6.0f;
	const float headY = top + radius;       // 丸の中心  
	const float tipY = top + PinHeadHeight; // とがった先

	drawList->AddTriangleFilled(
		ImVec2(x - radius * 0.8f, headY + 2.0f),
		ImVec2(x + radius * 0.8f, headY + 2.0f),
		ImVec2(x, tipY),
		PinColor);
	drawList->AddCircleFilled(ImVec2(x, headY), radius, PinColor);
}
void StudioTimelineWidget::DrawRangeHandle(ImDrawList* drawList, float x, float handleY, bool highlighted) const
{
	const ImU32 color = highlighted ? RangeHandleActiveColor : RangeHandleColor;
	const float radius = 6.0f;
	const float headY = handleY + RangeHandleHeight - radius; // 丸の中心

	// 線：頭のとがった先から、目盛りの帯の上端まで
	drawList->AddLine(ImVec2(x, handleY), ImVec2(x, top + PinHeadHeight), color, 1.5f);

	// 頭：上向きにとがった形
	drawList->AddTriangleFilled(
		ImVec2(x - radius * 0.8f, headY - 2.0f),
		ImVec2(x + radius * 0.8f, headY - 2.0f),
		ImVec2(x, handleY),
		color);
	drawList->AddCircleFilled(ImVec2(x, headY), radius, color);
}
