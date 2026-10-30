#include "StudioAnimationEdit.h"
#include "Engine/ECS/Components/SkinnedMeshComponent.h"
#include "Engine/ECS/Components/AnimatorComponent.h"
#include "Engine/Resource/AnimationEditBaker.h"
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <cstdio>
#include <cmath>

namespace
{
	// rootの位置キーから、グラフ用の値を作る
	RootMotionCurve BuildCurve(const Model::Animation& animation, int rootIndex)
	{
		RootMotionCurve curve;
		if (rootIndex < 0 || rootIndex >= (int)animation.nodeAnims.size()) return curve;

		const auto& keys = animation.nodeAnims[rootIndex].positionKeyframes;
		if (keys.empty()) return curve;

		float distance = 0.0f;
		for (size_t i = 0; i < keys.size(); i++)
		{
			// 水平に進んだ距離を積み上げる
			if (i > 0)
			{
				const float dx = keys[i].value.x - keys[i - 1].value.x;
				const float dz = keys[i].value.z - keys[i - 1].value.z;
				distance += std::sqrt(dx * dx + dz * dz);
			}

			curve.times.push_back(keys[i].seconds);
			curve.xz.push_back(distance);
			curve.y.push_back(keys[i].value.y - keys.front().value.y);
		}
		return curve;
	}

	// 点線を描く
	void AddDashedPolyline(ImDrawList* drawList, const std::vector<ImVec2>& points, ImU32 color, float thickness)
	{
		const float dash = 6.0f;
		const float gap = 4.0f;
		const float period = dash + gap;
		float pattern = 0.0f; // 点線の模様のどこまで進んだか

		for (size_t i = 1; i < points.size(); i++)
		{
			const ImVec2 a = points[i - 1];
			const ImVec2 b = points[i];
			const float segLength = std::sqrt((b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y));
			if (segLength <= 0.0f) continue;

			float done = 0.0f;
			while (done < segLength)
			{
				const float phase = std::fmod(pattern, period);
				const bool drawing = (phase < dash);
				const float remain = drawing ? (dash - phase) : (period - phase);
				const float step = (std::min)(remain, segLength - done);

				if (drawing)
				{
					const float t0 = done / segLength;
					const float t1 = (done + step) / segLength;
					drawList->AddLine(
						ImVec2(a.x + (b.x - a.x) * t0, a.y + (b.y - a.y) * t0),
						ImVec2(a.x + (b.x - a.x) * t1, a.y + (b.y - a.y) * t1),
						color, thickness);
				}

				done += step;
				pattern += step;
			}
		}
	}

	// グラフの値を、指定時刻で補間して取り出す
	float SampleCurve(const std::vector<float>& times, const std::vector<float>& values, float t)
	{
		if (values.empty()) return 0.0f;
		if (t <= times.front()) return values.front();

		for (size_t i = 1; i < times.size(); i++)
		{
			if (t <= times[i])
			{
				const float span = times[i] - times[i - 1];
				const float rate = (span > 0.0f) ? (t - times[i - 1]) / span : 0.0f;
				return values[i - 1] + (values[i] - values[i - 1]) * rate;
			}
		}
		return values.back();
	}

	// XZ：区間の中で水平に進んだ距離
	float MeasureXZ(const RootMotionCurve& curve, float start, float end)
	{
		return SampleCurve(curve.times, curve.xz, end) - SampleCurve(curve.times, curve.xz, start);
	}

	// Y：区間の中での最高点
	float MeasureYPeak(const RootMotionCurve& curve, float start, float end)
	{
		const float base = SampleCurve(curve.times, curve.y, start);
		float peak = SampleCurve(curve.times, curve.y, end) - base;

		for (size_t i = 0; i < curve.times.size(); i++)
		{
			if (curve.times[i] <= start || curve.times[i] >= end) continue;
			const float d = curve.y[i] - base;
			if (std::fabs(d) > std::fabs(peak)) peak = d;
		}
		return peak;
	}
}


// ImGui描画
void StudioAnimationEdit::DrawGUI(entt::registry& registry, StudioContext& context)
{
	if (!context.showAnimationEdit) return;

	std::string panelName = "Animation Edit##" + std::to_string(windowId);
	ImGui::Begin(panelName.c_str());

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

	// モデルが切り替わったら、そのモデル用の設定を読み込み直す
	if (loadedModelPath != context.currentModelPath)
	{
		Load(context.currentModelPath, *skinned.model);
	}

	if (animator.currentAnimation.empty())
	{
		ImGui::TextDisabled("Select an animation from the Animator panel.");
		ImGui::End();
		return;
	}

	// モードと焼き方を合わせる
	SyncBakeWithMode(*skinned.model, animator, context);

	const std::string& clipName = animator.currentAnimation;
	const float detailWidth = 260.0f;

	// 左：タイムライン
	ImGui::BeginChild("AnimEditTimeline", ImVec2(-detailWidth, 0.0f));
	bool changed = DrawTimelineArea(*skinned.model, animator, context);
	ImGui::EndChild();

	ImGui::SameLine();

	// 右：数値
	ImGui::BeginChild("AnimEditDetail", ImVec2(detailWidth, 0.0f), true);
	changed |= DrawDetailArea(*skinned.model, clipName, context.currentModelPath);
	ImGui::EndChild();

	// どちらかで値が変わったら、生データから焼き直す
	if (changed)
	{
		Rebake(*skinned.model);
		context.requestPoseUpdate = true;
		isDirty = true;
		curveDirty = true;
	}

	ImGui::End();
}



// タイムライン
bool StudioAnimationEdit::DrawTimelineArea(const Model& model, AnimatorComponent& animator, StudioContext& context)
{
	const std::string& clipName = animator.currentAnimation;

	// モード切り替え
	if (ImGui::RadioButton("Trim", mode == EditMode::Trim)) mode = EditMode::Trim;
	ImGui::SameLine();
	if (ImGui::RadioButton("Root Motion", mode == EditMode::RootMotion)) mode = EditMode::RootMotion;

	// ルートモーションモードでは、表示する軸を切り替える
	if (mode == EditMode::RootMotion)
	{
		auto setViewAxis = [&](SectionAxis axis)
			{
				viewAxis = axis;
				// 見えなくなる軸の区間を選んでいたら、選択を外す
				if (selectedAxis != viewAxis) selectedSection = -1;
			};

		ImGui::SameLine();
		ImGui::TextDisabled("|");
		ImGui::SameLine();
		if (ImGui::RadioButton("XZ##ViewAxis", viewAxis == SectionAxis::XZ)) setViewAxis(SectionAxis::XZ);
		ImGui::SameLine();
		if (ImGui::RadioButton("Y##ViewAxis", viewAxis == SectionAxis::Y)) setViewAxis(SectionAxis::Y);
	}
	ImGui::Separator();

	// 仮：段階5までは、焼き込み後のクリップの長さで表示する
	float length = 0.0f;
	for (const auto& anim : model.GetAnimations())
	{
		if (anim.name == clipName)
		{
			length = anim.secondsLength;
			break;
		}
	}

	const float frameStep = CalcFrameStep(model, clipName);

	// 再生ボタンと現在時刻
	if (mode == EditMode::Trim)
	{
		// トリミングモード：使う範囲だけを、このツールが時刻を進めて再生する
		float trimStart = 0.0f;
		float trimEnd = 0.0f;
		GetTrimRange(clipName, length, trimStart, trimEnd);

		if (StudioTimelineWidget::DrawToolbar(rangePlaying, animator.animationTime, frameStep))
		{
			if (rangePlaying)
			{
				// 範囲の外から再生したら、Start から
				if (animator.animationTime < trimStart || animator.animationTime >= trimEnd)
				{
					animator.animationTime = trimStart;
				}
				animator.isPlaying = false; // Animator には進めさせない
				animator.isEnd = false;
				context.requestPoseUpdate = true;
			}
		}

		if (rangePlaying)
		{
			AdvanceRangePlayback(animator, context, trimStart, trimEnd);
		}
	}
	else
	{
		// ルートモーションモード：Animator に再生させる
		if (StudioTimelineWidget::DrawToolbar(animator.isPlaying, animator.animationTime, frameStep))
		{
			// 最後まで再生しきった状態から再生したら、頭に戻す
			if (animator.isPlaying && animator.animationTime >= length)
			{
				animator.animationTime = 0.0f;
			}
			animator.isEnd = false;
		}
	}

	// タイムライン
	if (timeline.Begin("##AnimEditRuler", length, frameStep, animator.animationTime))
	{
		animator.isPlaying = false;
		animator.isEnd = false;
		rangePlaying = false;
		context.requestPoseUpdate = true;
	}

	// トラック
	bool changed = false;
	if (mode == EditMode::Trim)
	{
		changed |= DrawTrimTrack(clipName, length, frameStep);
	}
	else
	{
		changed |= DrawRootMotionTracks(model, clipName, length, frameStep);
	}

	timeline.End();

	// ピンの位置で操作するボタン
	if (mode == EditMode::Trim)
	{
		changed |= DrawTrimButtons(clipName, length, frameStep, animator.animationTime);
	}
	else
	{
		changed |= DrawSectionButtons(clipName, length, frameStep, animator.animationTime);
	}

	return changed;
}
// 数値
bool StudioAnimationEdit::DrawDetailArea(const Model& model, const std::string& clipName, const std::string& modelPath)
{
	// モデル全体の設定
	bool changed = DrawRootNodeCombo(model);
	ImGui::Separator();

	// クリップごとの設定
	ImGui::Text("Clip: %s", clipName.c_str());
	changed |= DrawClipSettings(model, clipName);

	ImGui::Separator();

	// リセット：このクリップの設定を消す
	if (editData.FindClip(clipName) && ImGui::Button("Reset Clip"))
	{
		auto& clips = editData.clips;
		clips.erase(
			std::remove_if(clips.begin(), clips.end(),
				[&](const AnimationEditClipData& c) { return c.clipName == clipName; }),
			clips.end());
		changed = true;
	}

	// 保存
	if (ImGui::Button("Save"))
	{
		Save(modelPath);
	}
	if (isDirty)
	{
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "* unsaved");
	}

	return changed;
}



// トリミングのトラック
bool StudioAnimationEdit::DrawTrimTrack(const std::string& clipName, float rawLength, float frameStep)
{
	float start = 0.0f;
	float end = 0.0f;
	GetTrimRange(clipName, rawLength, start, end);

	ImDrawList* drawList = ImGui::GetWindowDrawList();
	const float margin = 4.0f;
	const float trackHeight = 28.0f;
	const float top = ImGui::GetCursorScreenPos().y + margin;
	const float bottom = top + trackHeight;

	// 元の長さ全体（暗い帯）
	drawList->AddRectFilled(ImVec2(timeline.GetLeft(), top), ImVec2(timeline.GetRight(), bottom), IM_COL32(50, 52, 56, 255), 3.0f);
	// 使う範囲（明るい帯）
	drawList->AddRectFilled(ImVec2(timeline.TimeToX(start), top), ImVec2(timeline.TimeToX(end), bottom), IM_COL32(79, 195, 247, 200), 3.0f);

	// 範囲の端のピン
	const float minLength = (frameStep > 0.0f) ? frameStep : 0.001f;
	bool changed = false;
	if (timeline.DrawRangeHandles("##TrimRange", bottom, frameStep, minLength, start, end))
	{
		changed = SetTrimRange(clipName, rawLength, start, end);
	}

	// 帯とピンの頭の分の高さを確保する
	ImGui::Dummy(ImVec2(0.0f, margin + trackHeight + StudioTimelineWidget::RangeHandleHeight + margin));

	return changed;
}
// トリミングのボタン
bool StudioAnimationEdit::DrawTrimButtons(const std::string& clipName, float rawLength, float frameStep, float pinTime)
{
	float start = 0.0f;
	float end = 0.0f;
	GetTrimRange(clipName, rawLength, start, end);

	// 最低1フレーム分の長さを残す
	const float minLength = (frameStep > 0.0f) ? frameStep : 0.001f;
	const float tolerance = minLength * 0.5f;

	bool changed = false;

	ImGui::BeginDisabled(pinTime > end - minLength + tolerance);
	if (ImGui::Button("Set Start"))
	{
		changed |= SetTrimRange(clipName, rawLength, pinTime, end);
	}
	ImGui::EndDisabled();

	ImGui::SameLine();

	ImGui::BeginDisabled(pinTime < start + minLength - tolerance);
	if (ImGui::Button("Set End"))
	{
		changed |= SetTrimRange(clipName, rawLength, start, pinTime);
	}
	ImGui::EndDisabled();

	return changed;
}



// ルートモーションのグラフ
bool StudioAnimationEdit::DrawRootMotionTracks(const Model& model, const std::string& clipName, float length, float frameStep)
{
	// クリップが変わったら、区間の選択を外す
	if (curveClipName != clipName)
	{
		selectedSection = -1;
	}

	// 必要なときだけ作り直す
	if (curveDirty || curveClipName != clipName)
	{
		RebuildCurves(model, clipName);
	}

	const float trimStart = GetTrimStart(clipName);
	bool changed = false;
	float tintStart = -1.0f;
	float tintEnd = -1.0f;

	// 表示している軸だけ表示
	const bool isY = (viewAxis == SectionAxis::Y);
	const ImU32 color = isY ? IM_COL32(255, 183, 77, 255) : IM_COL32(79, 195, 247, 255);

	GetSelectedSectionRange(viewAxis, clipName, trimStart, tintStart, tintEnd);
	const float graphTop = DrawCurveTrack(isY ? "Y" : "XZ", isY, 160.0f, color, tintStart, tintEnd);
	changed |= DrawSectionBand(isY ? "##SectionsY" : "##SectionsXZ", viewAxis, clipName, trimStart, length, frameStep, graphTop);

	return changed;
}
// グラフの値を作り直す
void StudioAnimationEdit::RebuildCurves(const Model& model, const std::string& clipName)
{
	originalCurve = RootMotionCurve{};
	editedCurve = RootMotionCurve{};
	curveClipName = clipName;
	curveDirty = false;

	const int rootIndex = AnimationEditBaker::FindNodeIndex(model.GetNodes(), editData.rootNodeName);
	if (rootIndex < 0) return;

	// 編集後：焼き込み後のクリップ
	for (const auto& anim : model.GetAnimations())
	{
		if (anim.name == clipName)
		{
			editedCurve = BuildCurve(anim, rootIndex);
			break;
		}
	}

	// 元：同じ設定から、区間だけを外して焼いたもの
	for (const auto& raw : model.GetRawAnimations())
	{
		if (raw.name != clipName) continue;

		const AnimationEditClipData* existing = editData.FindClip(clipName);
		AnimationEditClipData withoutSections = existing ? *existing : AnimationEditClipData{};
		withoutSections.sectionsXZ.clear();
		withoutSections.sectionsY.clear();

		const Model::Animation baked = AnimationEditBaker::BakeClip(raw, model.GetNodes(), rootIndex, &withoutSections);
		originalCurve = BuildCurve(baked, rootIndex);
		break;
	}
}
// グラフのトラックを1本描く
float StudioAnimationEdit::DrawCurveTrack(const char* label, bool isY, float height, ImU32 color, float tintStart, float tintEnd)
{
	const std::vector<float>& original = isY ? originalCurve.y : originalCurve.xz;
	const std::vector<float>& edited = isY ? editedCurve.y : editedCurve.xz;

	// 縦の範囲
	float minValue = 0.0f;
	float maxValue = 0.0f;
	for (float v : original) { minValue = (std::min)(minValue, v); maxValue = (std::max)(maxValue, v); }
	for (float v : edited) { minValue = (std::min)(minValue, v); maxValue = (std::max)(maxValue, v); }

	// ほとんど動かないときは、線が真ん中に来るように範囲を広げる
	if (maxValue - minValue < 0.1f)
	{
		const float center = (minValue + maxValue) * 0.5f;
		minValue = center - 0.05f;
		maxValue = center + 0.05f;
	}

	ImDrawList* drawList = ImGui::GetWindowDrawList();
	const float margin = 4.0f;
	const float padding = 6.0f; // 線が枠に張り付かないための余白
	const float top = ImGui::GetCursorScreenPos().y + margin;
	const float bottom = top + height;

	// 背景
	drawList->AddRectFilled(ImVec2(timeline.GetLeft(), top), ImVec2(timeline.GetRight(), bottom), IM_COL32(43, 45, 49, 255), 3.0f);

	// 画面のY座標
	const float usableHeight = height - padding * 2.0f;
	auto toY = [&](float value)
		{
			const float rate = (value - minValue) / (maxValue - minValue);
			return bottom - padding - rate * usableHeight;
		};

	// 横線の目盛り
	const float candidates[] = { 0.01f, 0.02f, 0.05f, 0.1f, 0.2f, 0.5f, 1.0f, 2.0f, 5.0f, 10.0f };
	float interval = candidates[9];
	for (float c : candidates)
	{
		if (usableHeight * c / (maxValue - minValue) >= 24.0f)
		{
			interval = c;
			break;
		}
	}

	// 選んでいる区間の範囲を薄く色付けする
	if (tintEnd > tintStart)
	{
		const float x0 = (std::max)(timeline.TimeToX(tintStart), timeline.GetLeft());
		const float x1 = (std::min)(timeline.TimeToX(tintEnd), timeline.GetRight());
		if (x1 > x0)
		{
			const ImU32 tintColor = (color & 0x00FFFFFF) | (40u << 24); // 線の色を薄くしたもの
			drawList->AddRectFilled(ImVec2(x0, top), ImVec2(x1, bottom), tintColor);
		}
	}

	// 間隔に合わせて小数の桁を決める
	const char* gridFormat = (interval < 0.1f) ? "%.2f" : (interval < 1.0f) ? "%.1f" : "%.0f";

	const int firstLine = (int)std::ceil(minValue / interval - 0.0001f);
	const int lastLine = (int)std::floor(maxValue / interval + 0.0001f);
	for (int i = firstLine; i <= lastLine; i++)
	{
		const float value = i * interval;
		const float y = toY(value);

		// 0m の線は少し明るくして、基準にする
		const ImU32 lineColor = (i == 0) ? IM_COL32(90, 92, 98, 255) : IM_COL32(60, 62, 68, 255);
		drawList->AddLine(ImVec2(timeline.GetLeft(), y), ImVec2(timeline.GetRight(), y), lineColor, 1.0f);

		// 値のラベル（右端、線の上。上にはみ出すときは線の下）
		char gridLabel[16];
		snprintf(gridLabel, sizeof(gridLabel), gridFormat, value);
		const ImVec2 textSize = ImGui::CalcTextSize(gridLabel);
		const float labelY = (y - textSize.y - 1.0f < top) ? (y + 1.0f) : (y - textSize.y - 1.0f);
		drawList->AddText(ImVec2(timeline.GetRight() - textSize.x - 4.0f, labelY), IM_COL32(130, 130, 130, 255), gridLabel);
	}

	// 画面の点
	auto toPoints = [&](const std::vector<float>& times, const std::vector<float>& values)
		{
			std::vector<ImVec2> points;
			points.reserve(values.size());
			for (size_t i = 0; i < values.size(); i++)
			{
				points.push_back(ImVec2(timeline.TimeToX(times[i]), toY(values[i])));
			}
			return points;
		};

	// 元（点線）を先に、編集後（実線）を上に重ねる
	const std::vector<ImVec2> originalPoints = toPoints(originalCurve.times, original);
	const std::vector<ImVec2> editedPoints = toPoints(editedCurve.times, edited);
	AddDashedPolyline(drawList, originalPoints, IM_COL32(140, 140, 140, 255), 1.5f);
	if (editedPoints.size() >= 2)
	{
		drawList->AddPolyline(editedPoints.data(), (int)editedPoints.size(), color, 0, 2.0f);
	}

	// ラベル：トラック名と、最後の値
	char text[32];
	snprintf(text, sizeof(text), "%s  %.2f m", label, edited.empty() ? 0.0f : edited.back());
	drawList->AddText(ImVec2(timeline.GetLeft() + 4.0f, top + 2.0f), IM_COL32(200, 200, 200, 255), text);

	// 描いた分の高さを確保する
	ImGui::Dummy(ImVec2(0.0f, height + margin * 2.0f));

	return top; 
}
// 区間の帯を描いて、クリックで選択する
bool StudioAnimationEdit::DrawSectionBand(const char* id, SectionAxis axis, const std::string& clipName, float trimStart, float length, float frameStep, float graphTop)
{
	const AnimationEditClipData* clip = editData.FindClip(clipName);
	const std::vector<AnimationEditSection>* sections = nullptr;
	if (clip) sections = (axis == SectionAxis::XZ) ? &clip->sectionsXZ : &clip->sectionsY;

	const ImU32 color = (axis == SectionAxis::XZ) ? IM_COL32(79, 195, 247, 255) : IM_COL32(255, 183, 77, 255);
	const ImU32 dimColor = (axis == SectionAxis::XZ) ? IM_COL32(79, 195, 247, 110) : IM_COL32(255, 183, 77, 110);

	ImDrawList* drawList = ImGui::GetWindowDrawList();
	const float margin = 2.0f;
	const float bandHeight = 22.0f;
	const ImVec2 cursor = ImGui::GetCursorScreenPos();
	const float top = cursor.y + margin;
	const float bottom = top + bandHeight;

	// 帯の全体をクリックできるようにする
	ImGui::SetCursorScreenPos(ImVec2(timeline.GetLeft(), top));
	const bool clicked = ImGui::InvisibleButton(id, ImVec2(timeline.GetRight() - timeline.GetLeft(), bandHeight));
	const float mouseX = ImGui::GetIO().MousePos.x;

	// 背景
	drawList->AddRectFilled(ImVec2(timeline.GetLeft(), top), ImVec2(timeline.GetRight(), bottom), IM_COL32(50, 52, 56, 255), 3.0f);

	int hitIndex = -1;
	if (sections)
	{
		for (int i = 0; i < (int)sections->size(); i++)
		{
			const AnimationEditSection& s = (*sections)[i];

			// 区間の時間を、カット後の時間に直す
			const float start = s.start - trimStart;
			const float end = s.end - trimStart;

			// カットの範囲外にはみ出した部分は描かない
			const float x0 = (std::max)(timeline.TimeToX(start), timeline.GetLeft());
			const float x1 = (std::min)(timeline.TimeToX(end), timeline.GetRight());
			if (x1 <= x0) continue;

			const bool selected = (selectedAxis == axis && selectedSection == i);
			drawList->AddRectFilled(ImVec2(x0, top + 2.0f), ImVec2(x1, bottom - 2.0f), selected ? color : dimColor, 3.0f);

			// 距離のラベル
			const float value = (axis == SectionAxis::XZ) ? MeasureXZ(editedCurve, start, end) : MeasureYPeak(editedCurve, start, end);
			char label[32];
			snprintf(label, sizeof(label), "%.2f m", value);
			const ImVec2 textSize = ImGui::CalcTextSize(label);
			if (textSize.x + 6.0f < x1 - x0)
			{
				drawList->AddText(
					ImVec2((x0 + x1 - textSize.x) * 0.5f, (top + bottom - textSize.y) * 0.5f),
					selected ? IM_COL32(0, 0, 0, 255) : IM_COL32(220, 220, 220, 255),
					label);
			}

			if (mouseX >= x0 && mouseX <= x1) hitIndex = i;
		}
	}

	// クリックした所の区間を選ぶ
	if (clicked)
	{
		selectedAxis = axis;
		selectedSection = hitIndex;
	}

	// 選んでいる区間の端のピン
	bool changed = false;
	if (sections && selectedAxis == axis && selectedSection >= 0 && selectedSection < (int)sections->size())
	{
		const AnimationEditSection& s = (*sections)[selectedSection];
		float start = std::clamp(s.start - trimStart, 0.0f, length);
		float end = std::clamp(s.end - trimStart, 0.0f, length);

		// 隣の区間にぶつかったら止まる
		float lower = 0.0f;
		float upper = length;
		GetSectionLimits(*sections, selectedSection, trimStart, length, lower, upper);

		const float minLength = (frameStep > 0.0f) ? frameStep : 0.001f;
		const std::string handleId = std::string(id) + "Handles";
		if (timeline.DrawRangeHandles(handleId.c_str(), bottom, frameStep, minLength, start, end, lower, upper, graphTop))
		{
			// 生データの時間に戻して書き込む
			AnimationEditClipData* editable = editData.FindOrCreateClip(clipName);
			auto& list = (axis == SectionAxis::XZ) ? editable->sectionsXZ : editable->sectionsY;
			list[selectedSection].start = start + trimStart;
			list[selectedSection].end = end + trimStart;
			changed = true;
		}
	}

	// 帯とピンの頭の分の高さを確保する
	ImGui::SetCursorScreenPos(ImVec2(cursor.x, bottom + StudioTimelineWidget::RangeHandleHeight + margin));
	ImGui::Dummy(ImVec2(0.0f, 0.0f));

	return changed;
}
// 選んでいる区間の範囲
bool StudioAnimationEdit::GetSelectedSectionRange(SectionAxis axis, const std::string& clipName, float trimStart, float& outStart, float& outEnd) const
{
	outStart = -1.0f;
	outEnd = -1.0f;
	if (selectedAxis != axis || selectedSection < 0) return false;

	const AnimationEditClipData* clip = editData.FindClip(clipName);
	if (!clip) return false;

	const auto& sections = (axis == SectionAxis::XZ) ? clip->sectionsXZ : clip->sectionsY;
	if (selectedSection >= (int)sections.size()) return false;

	outStart = sections[selectedSection].start - trimStart;
	outEnd = sections[selectedSection].end - trimStart;
	return true;
}
// 今のクリップのカットの始まり
float StudioAnimationEdit::GetTrimStart(const std::string& clipName) const
{
	const AnimationEditClipData* clip = editData.FindClip(clipName);
	return clip ? (std::max)(clip->trimStart, 0.0f) : 0.0f;
}
// 区間の追加とSet Start / Set End
bool StudioAnimationEdit::DrawSectionButtons(const std::string& clipName, float length, float frameStep, float pinTime)
{
	const float trimStart = GetTrimStart(clipName);
	const float minLength = (frameStep > 0.0f) ? frameStep : 0.001f;
	const float tolerance = minLength * 0.5f; // 小数の誤差として許す量

	const AnimationEditClipData* clip = editData.FindClip(clipName);
	bool changed = false;

	// ピンの位置に区間を追加できるか
	auto canAdd = [&](const std::vector<AnimationEditSection>* list, float& outEnd)
		{
			float end = length;
			if (list)
			{
				for (const auto& s : *list)
				{
					const float st = s.start - trimStart;
					const float en = s.end - trimStart;
					if (pinTime >= st - tolerance && pinTime < en - tolerance) return false; // 既存の区間の中
					if (st >= pinTime - tolerance) end = (std::min)(end, st);                // 次の区間の始まりまで
				}
			}
			outEnd = end;
			return (end - pinTime) >= (minLength - tolerance);
		};

	// 区間を追加して、作った区間を選ぶ
	auto addSection = [&](SectionAxis axis, float end)
		{
			AnimationEditClipData* editable = editData.FindOrCreateClip(clipName);
			auto& list = (axis == SectionAxis::XZ) ? editable->sectionsXZ : editable->sectionsY;

			AnimationEditSection section;
			section.start = pinTime + trimStart; // 生データの時間で保存する
			section.end = end + trimStart;
			section.scale = 1.0f;

			// 開始時刻の順になるように差し込む
			auto it = std::find_if(list.begin(), list.end(),
				[&](const AnimationEditSection& s) { return s.start > section.start; });
			const int index = (int)(it - list.begin());
			list.insert(it, section);

			selectedAxis = axis;
			selectedSection = index;
			changed = true;
		};

	// 追加
	{
		const std::vector<AnimationEditSection>* list = nullptr;
		if (clip) list = (viewAxis == SectionAxis::XZ) ? &clip->sectionsXZ : &clip->sectionsY;

		float end = 0.0f;
		const bool enabled = canAdd(list, end);

		ImGui::BeginDisabled(!enabled);
		if (ImGui::Button("Add Section"))
		{
			addSection(viewAxis, end);
			clip = editData.FindClip(clipName); // 追加で中身が変わったので取り直す
		}
		ImGui::EndDisabled();
	}

	// 選んでいる区間の SetStart/SetEnd
	const std::vector<AnimationEditSection>* selectedList = nullptr;
	if (clip) selectedList = (selectedAxis == SectionAxis::XZ) ? &clip->sectionsXZ : &clip->sectionsY;
	const bool hasSelection = selectedList && selectedSection >= 0 && selectedSection < (int)selectedList->size();

	float start = 0.0f;
	float end = 0.0f;
	float lower = 0.0f;
	float upper = length;
	if (hasSelection)
	{
		start = (*selectedList)[selectedSection].start - trimStart;
		end = (*selectedList)[selectedSection].end - trimStart;
		GetSectionLimits(*selectedList, selectedSection, trimStart, length, lower, upper);
	}

	ImGui::SameLine();
	ImGui::Dummy(ImVec2(16.0f, 0.0f)); // 追加ボタンとの間を空ける
	ImGui::SameLine();

	// Set Start：前の区間に食い込まず、終わりとの間を1フレーム残す
	ImGui::BeginDisabled(!hasSelection || pinTime < lower - tolerance || pinTime > end - minLength + tolerance);
	if (ImGui::Button("Set Start##Section"))
	{
		AnimationEditClipData* editable = editData.FindOrCreateClip(clipName);
		auto& list = (selectedAxis == SectionAxis::XZ) ? editable->sectionsXZ : editable->sectionsY;
		list[selectedSection].start = pinTime + trimStart;
		changed = true;
	}
	ImGui::EndDisabled();

	ImGui::SameLine();

	// Set End：後ろの区間に食い込まず、始まりとの間を1フレーム残す
	ImGui::BeginDisabled(!hasSelection || pinTime > upper + tolerance || pinTime < start + minLength - tolerance);
	if (ImGui::Button("Set End##Section"))
	{
		AnimationEditClipData* editable = editData.FindOrCreateClip(clipName);
		auto& list = (selectedAxis == SectionAxis::XZ) ? editable->sectionsXZ : editable->sectionsY;
		list[selectedSection].end = pinTime + trimStart;
		changed = true;
	}
	ImGui::EndDisabled();

	return changed;
}
// 区間が動ける範囲
void StudioAnimationEdit::GetSectionLimits(const std::vector<AnimationEditSection>& sections, int index, float trimStart, float length, float& outLower, float& outUpper)
{
	outLower = 0.0f;
	outUpper = length;

	const float start = sections[index].start - trimStart;
	const float end = sections[index].end - trimStart;

	for (int i = 0; i < (int)sections.size(); i++)
	{
		if (i == index) continue;
		const float otherStart = sections[i].start - trimStart;
		const float otherEnd = sections[i].end - trimStart;
		if (otherEnd <= start + 0.0001f) outLower = (std::max)(outLower, otherEnd);   // 前にある区間
		if (otherStart >= end - 0.0001f) outUpper = (std::min)(outUpper, otherStart); // 後ろにある区間
	}
}


// Root Node の選択
bool StudioAnimationEdit::DrawRootNodeCombo(const Model& model)
{
	bool changed = false;

	if (ImGui::BeginCombo("Root Node", editData.rootNodeName.c_str()))
	{
		// 一覧と自動の値は、開いている間だけ調べる
		const auto& nodes = model.GetNodes();
		const std::string defaultName = AnimationEditBaker::FindDefaultRootNodeName(model);

		for (int index : AnimationEditBaker::CollectBoneIndices(model))
		{
			const std::string& name = nodes[index].name;
			const bool selected = (name == editData.rootNodeName);

			// 自動で見つけたボーンには目印を付ける
			const std::string label = (name == defaultName) ? name + "  (auto)" : name;

			if (ImGui::Selectable(label.c_str(), selected) && !selected)
			{
				editData.rootNodeName = name;
				changed = true;
			}
			if (selected) ImGui::SetItemDefaultFocus();
		}

		ImGui::EndCombo();
	}

	return changed;
}
// クリップの設定
bool StudioAnimationEdit::DrawClipSettings(const Model& model, const std::string& clipName)
{
	// 設定が無いクリップは既定値で表示する
	const AnimationEditClipData* existing = editData.FindClip(clipName);
	AnimationEditClipData clip = existing ? *existing : AnimationEditClipData{};
	clip.clipName = clipName;

	bool changed = false;

	// 向きの補正
	ImGui::Separator();
	ImGui::TextDisabled("Orientation");
	changed |= ImGui::DragFloat("Yaw Offset", &clip.yawOffsetDegrees, 1.0f, -180.0f, 180.0f, "%.1f deg");

	// モードごとの項目
	ImGui::Separator();
	if (mode == EditMode::Trim)
	{
		ImGui::TextDisabled("Trimming");

		const float rawLength = FindRawLength(model, clipName);
		const float frameStep = CalcFrameStep(model, clipName);
		const float minLength = (frameStep > 0.0f) ? frameStep : 0.001f;
		const float dragSpeed = (frameStep > 0.0f) ? frameStep : 0.001f;

		float start = clip.trimStart;
		float end = (clip.trimEnd < 0.0f) ? rawLength : clip.trimEnd;

		bool trimChanged = false;
		trimChanged |= ImGui::DragFloat("Start", &start, dragSpeed, 0.0f, end - minLength, "%.3f s");
		trimChanged |= ImGui::DragFloat("End", &end, dragSpeed, start + minLength, rawLength, "%.3f s");

		if (trimChanged)
		{
			// フレームに吸い付かせてから書き戻す
			start = StudioTimelineWidget::SnapToFrame(start, frameStep, rawLength);
			end = StudioTimelineWidget::SnapToFrame(end, frameStep, rawLength);
			clip.trimStart = start;
			clip.trimEnd = (end >= rawLength - 0.0001f) ? -1.0f : end; // 最後までなら「最後まで」として保存
			changed = true;
		}

		// フレームと長さの表示
		const int startFrame = (frameStep > 0.0f) ? (int)std::round(start / frameStep) : 0;
		const int endFrame = (frameStep > 0.0f) ? (int)std::round(end / frameStep) : 0;
		ImGui::TextDisabled("F%d - F%d", startFrame, endFrame);
		ImGui::TextDisabled("Length %.3f s (raw %.3f s)", end - start, rawLength);
	}
	else
	{
		ImGui::TextDisabled((viewAxis == SectionAxis::XZ) ? "Section (XZ)" : "Section (Y)");
		changed |= DrawSectionSettings(model, clip);
	}

	// 変更があったときだけ、データに書き戻す
	if (changed)
	{
		*editData.FindOrCreateClip(clipName) = clip;
	}

	return changed;
}
// 選んでいる区間の数値
bool StudioAnimationEdit::DrawSectionSettings(const Model& model, AnimationEditClipData& clip)
{
	auto& list = (selectedAxis == SectionAxis::XZ) ? clip.sectionsXZ : clip.sectionsY;
	if (selectedAxis != viewAxis || selectedSection < 0 || selectedSection >= (int)list.size())
	{
		ImGui::TextDisabled("No section selected.");
		return false;
	}

	AnimationEditSection& section = list[selectedSection];
	const bool isY = (selectedAxis == SectionAxis::Y);

	// 範囲
	const float trimStart = (std::max)(clip.trimStart, 0.0f);
	const float start = section.start - trimStart;
	const float end = section.end - trimStart;
	const float frameStep = CalcFrameStep(model, clip.clipName);
	const int startFrame = (frameStep > 0.0f) ? (int)std::round(start / frameStep) : 0;
	const int endFrame = (frameStep > 0.0f) ? (int)std::round(end / frameStep) : 0;

	ImGui::Text("Range     %.3f - %.3f s", start, end);
	ImGui::TextDisabled("          F%d - F%d", startFrame, endFrame);

	// 元の値
	const float original = isY ? MeasureYPeak(originalCurve, start, end) : MeasureXZ(originalCurve, start, end);
	ImGui::Text(isY ? "Original peak  %.2f m" : "Original  %.2f m", original);

	bool changed = false;

	// 元がほぼ0なら、倍率が計算できないので入力できない
	const bool canEdit = std::fabs(original) > 0.001f;

	// 入力は「距離」、保存は「倍率」（距離 = 元の値 × 倍率）
	float target = original * section.scale;

	// 元と逆向きの値は入れられないようにする
	const float minValue = (original >= 0.0f) ? 0.0f : -100.0f;
	const float maxValue = (original >= 0.0f) ? 100.0f : 0.0f;

	ImGui::BeginDisabled(!canEdit);
	if (ImGui::DragFloat(isY ? "Peak" : "Distance", &target, 0.01f, minValue, maxValue, "%.2f m"))
	{
		section.scale = (std::max)(target / original, 0.0f);
		changed = true;
	}
	ImGui::EndDisabled();

	if (canEdit)
	{
		ImGui::TextDisabled("          x%.2f", section.scale);
	}
	else
	{
		ImGui::TextDisabled("No motion in this section.");
	}

	// 削除
	if (ImGui::Button("Delete Section"))
	{
		list.erase(list.begin() + selectedSection);
		selectedSection = -1;
		changed = true;
	}

	return changed;
}



// 使う範囲の中で時刻を進める
void StudioAnimationEdit::AdvanceRangePlayback(AnimatorComponent& animator, StudioContext& context, float start, float end)
{
	const float rangeLength = end - start;
	if (rangeLength <= 0.0f)
	{
		rangePlaying = false;
		return;
	}

	// 別のパネルの再生が始まったら、そちらを優先して止まる
	if (animator.isPlaying)
	{
		rangePlaying = false;
		return;
	}

	float t = animator.animationTime + ImGui::GetIO().DeltaTime * animator.playSpeed;

	// 範囲の終わりを越えたら、越えた分を残して Start 側へ戻す
	if (t >= end) t = start + std::fmod(t - start, rangeLength);
	if (t < start) t = start;

	animator.animationTime = t;
	context.requestPoseUpdate = true;
}
// 範囲を取り出す
void StudioAnimationEdit::GetTrimRange(const std::string& clipName, float rawLength, float& outStart, float& outEnd) const
{
	outStart = 0.0f;
	outEnd = rawLength;

	if (const AnimationEditClipData* clip = editData.FindClip(clipName))
	{
		outStart = std::clamp(clip->trimStart, 0.0f, rawLength);
		outEnd = (clip->trimEnd < 0.0f) ? rawLength : std::clamp(clip->trimEnd, outStart, rawLength);
	}
}
// 範囲を書き込む
bool StudioAnimationEdit::SetTrimRange(const std::string& clipName, float rawLength, float start, float end)
{
	// 最後までなら「-1（最後まで）」として保存する
	const float storedEnd = (end >= rawLength - 0.0001f) ? -1.0f : end;

	// 変わらないなら、設定を作らずに終わる
	const AnimationEditClipData* existing = editData.FindClip(clipName);
	const float oldStart = existing ? existing->trimStart : 0.0f;
	const float oldEnd = existing ? existing->trimEnd : -1.0f;
	if (oldStart == start && oldEnd == storedEnd) return false;

	AnimationEditClipData* clip = editData.FindOrCreateClip(clipName);
	clip->trimStart = start;
	clip->trimEnd = storedEnd;
	return true;
}



// 1フレームの秒数
float StudioAnimationEdit::CalcFrameStep(const Model& model, const std::string& clipName)
{
	for (const auto& anim : model.GetRawAnimations())
	{
		if (anim.name != clipName) continue;

		// 一番キーの多いチャンネルの間隔を、1フレームとみなす
		size_t maxKeys = 0;
		for (const auto& nodeAnim : anim.nodeAnims)
		{
			maxKeys = (std::max)(maxKeys, nodeAnim.positionKeyframes.size());
			maxKeys = (std::max)(maxKeys, nodeAnim.rotationKeyframes.size());
		}

		if (maxKeys < 2 || anim.secondsLength <= 0.0f) return 0.0f;
		return anim.secondsLength / (float)(maxKeys - 1);
	}
	return 0.0f;
}
// 生データの長さ
float StudioAnimationEdit::FindRawLength(const Model& model, const std::string& clipName)
{
	for (const auto& anim : model.GetRawAnimations())
	{
		if (anim.name == clipName) return anim.secondsLength;
	}
	return 0.0f;
}



// モードに合わせて焼き直す
void StudioAnimationEdit::Rebake(Model& model) const
{
	if (mode == EditMode::Trim)
	{
		// トリミングモード
		AnimationEditData preview = editData;
		for (auto& clip : preview.clips)
		{
			clip.trimStart = 0.0f;
			clip.trimEnd = -1.0f;
		}
		model.ApplyAnimationEdit(preview);
	}
	else
	{
		// ルートモーションモード
		model.ApplyAnimationEdit(editData);
	}
}
// モードと焼き方がずれていたら合わせる
void StudioAnimationEdit::SyncBakeWithMode(Model& model, AnimatorComponent& animator, StudioContext& context)
{
	const bool wantUntrimmed = (mode == EditMode::Trim);
	if (wantUntrimmed == bakedUntrimmed) return;

	// 今のクリップのカット範囲
	float trimStart = 0.0f;
	float trimEnd = -1.0f;
	if (const AnimationEditClipData* clip = editData.FindClip(animator.currentAnimation))
	{
		trimStart = clip->trimStart;
		trimEnd = clip->trimEnd;
	}

	const float rawLength = FindRawLength(model, animator.currentAnimation);
	if (trimEnd < 0.0f) trimEnd = rawLength;

	// ピンが同じポーズを指すように、時刻を変換する
	if (wantUntrimmed)
	{
		animator.animationTime += trimStart;
	}
	else
	{
		animator.animationTime = std::clamp(animator.animationTime - trimStart, 0.0f, trimEnd - trimStart);
	}

	Rebake(model);
	bakedUntrimmed = wantUntrimmed;
	rangePlaying = false; // モードが変わったら、範囲の再生は止め
	curveDirty = true;    // 焼き方が変わったので、グラフも作り直す
	context.requestPoseUpdate = true;
}



// 読み込み
void StudioAnimationEdit::Load(const std::string& modelPath, const Model& model)
{
	editData = AnimationEditData();

	std::ifstream ifs(AnimationEditData::GetFilePath(modelPath));
	if (ifs.is_open())
	{
		try
		{
			nlohmann::json j;
			ifs >> j;
			editData.FromJson(j);
		}
		catch (const std::exception&) {}
	}

	loadedModelPath = modelPath;
	isDirty = false;
	bakedUntrimmed = false; // 読み込んだばかりのモデルは、ResourceManagerで全部焼かれている
	curveDirty = true;
	selectedSection = -1;

	// ルートノードが未設定なら、モデルから探してファイルに書き込む
	if (editData.rootNodeName.empty())
	{
		editData.rootNodeName = AnimationEditBaker::FindDefaultRootNodeName(model);
		Save(modelPath);
	}
}
void StudioAnimationEdit::Save(const std::string& modelPath)
{
	// フォルダが無ければ作成する
	std::filesystem::create_directories("Data/Json/Animation");

	std::ofstream ofs(AnimationEditData::GetFilePath(modelPath));
	if (ofs.is_open())
	{
		ofs << editData.ToJson().dump(4);
		isDirty = false;
	}
}
