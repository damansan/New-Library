#include "StudioAnimationEdit.h"
#include "Engine/ECS/Components/SkinnedMeshComponent.h"
#include "Engine/ECS/Components/AnimatorComponent.h"
#include "Engine/Resource/AnimationEditBaker.h"
#include <algorithm>
#include <fstream>
#include <filesystem>
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
		ImGui::Dummy(ImVec2(0.0f, 80.0f)); // 段階6でグラフを追加する
	}

	timeline.End();

	// ピンの位置でカットする
	if (mode == EditMode::Trim) 
	{
		changed |= DrawTrimButtons(clipName, length, frameStep, animator.animationTime);
	}

	return changed;
}
// 数値
bool StudioAnimationEdit::DrawDetailArea(const Model& model, const std::string& clipName, const std::string& modelPath)
{
	ImGui::Text("Clip: %s", clipName.c_str());
	ImGui::TextDisabled("Root Node: %s", editData.rootNodeName.c_str());

	bool changed = DrawClipSettings(model, clipName);

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
void StudioAnimationEdit::DrawRootMotionTracks(const Model& model, const std::string& clipName)
{
}
// グラフの値を作り直す
void StudioAnimationEdit::RebuildCurves(const Model& model, const std::string& clipName)
{
}
// グラフのトラックを1本描く
void StudioAnimationEdit::DrawCurveTrack(const char* label, bool isY, float height, ImU32 color)
{
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
		ImGui::TextDisabled("Section");
		// 段階6で追加する
	}

	// 変更があったときだけ、データに書き戻す
	if (changed)
	{
		*editData.FindOrCreateClip(clipName) = clip;
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
	rangePlaying = false; // モードが変わったら、範囲の再生は止める
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
