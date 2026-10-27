#include "StudioAnimationEdit.h"
#include "Engine/ECS/Components/SkinnedMeshComponent.h"
#include "Engine/ECS/Components/AnimatorComponent.h"
#include "Engine/Resource/AnimationEditBaker.h"
#include <fstream>
#include <filesystem>

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

	// モデル共通の設定
	ImGui::Text("Root Node: %s", editData.rootNodeName.c_str());
	ImGui::Separator();

	// クリップごとの設定
	ImGui::Text("Clip: %s", animator.currentAnimation.c_str());

	if (DrawClipSettings(animator.currentAnimation))
	{
		// 値が変わったら、生データから焼き直す
		skinned.model->ApplyAnimationEdit(editData);
		context.requestPoseUpdate = true;
		isDirty = true;
	}

	ImGui::Separator();
	DrawRootMotionInfo(*skinned.model, animator.currentAnimation);

	ImGui::End();
}

// クリップ設定
bool StudioAnimationEdit::DrawClipSettings(const std::string& clipName)
{
	// 設定が無いクリップは既定値で表示する
	const AnimationEditClipData* existing = editData.FindClip(clipName);
	AnimationEditClipData clip = existing ? *existing : AnimationEditClipData{};
	clip.clipName = clipName;

	bool changed = false;

	// 向きの補正
	changed |= ImGui::DragFloat("Yaw Offset", &clip.yawOffsetDegrees, 1.0f, -180.0f, 180.0f, "%.1f deg");

	const float presets[] = { -90.0f, 0.0f, 90.0f, 180.0f };
	const char* presetLabels[] = { "-90", "0", "+90", "180" };
	for (int i = 0; i < 4; i++)
	{
		if (i > 0) ImGui::SameLine();
		if (ImGui::Button(presetLabels[i]))
		{
			clip.yawOffsetDegrees = presets[i];
			changed = true;
		}
	}

	// 移動量の倍率
	changed |= ImGui::DragFloat("Root Motion XZ", &clip.rootMotionScaleXZ, 0.01f, 0.0f, 10.0f, "%.2f");
	changed |= ImGui::DragFloat("Root Motion Y", &clip.rootMotionScaleY, 0.01f, 0.0f, 10.0f, "%.2f");

	// リセット
	if (existing && ImGui::Button("Reset Clip"))
	{
		auto& clips = editData.clips;
		clips.erase(
			std::remove_if(clips.begin(), clips.end(),
				[&](const AnimationEditClipData& c) { return c.clipName == clipName; }),
			clips.end());
		return true;
	}

	// 変更があったときだけ、データに書き戻す
	if (changed)
	{
		*editData.FindOrCreateClip(clipName) = clip;
	}

	return changed;
}

// 焼き込み後のルート移動量
void StudioAnimationEdit::DrawRootMotionInfo(const Model& model, const std::string& clipName)
{
	// ルートノードの番号を探す
	const auto& nodes = model.GetNodes();
	int rootIndex = -1;
	for (int i = 0; i < (int)nodes.size(); i++)
	{
		if (nodes[i].name == editData.rootNodeName) { rootIndex = i; break; }
	}
	if (rootIndex < 0)
	{
		ImGui::TextDisabled("Root node not found.");
		return;
	}

	// 焼き込み後のクリップを探す
	for (const auto& anim : model.GetAnimations())
	{
		if (anim.name != clipName) continue;

		if (rootIndex >= (int)anim.nodeAnims.size() || anim.nodeAnims[rootIndex].positionKeyframes.empty())
		{
			ImGui::TextDisabled("Root Motion: none");
			return;
		}

		// 最初と最後のキーの差＝1回再生したときの移動量
		const auto& keys = anim.nodeAnims[rootIndex].positionKeyframes;
		const DirectX::XMFLOAT3& a = keys.front().value;
		const DirectX::XMFLOAT3& b = keys.back().value;
		const float dx = b.x - a.x;
		const float dy = b.y - a.y;
		const float dz = b.z - a.z;
		const float horizontal = sqrtf(dx * dx + dz * dz);
		const float speed = (anim.secondsLength > 0.0f) ? horizontal / anim.secondsLength : 0.0f;

		ImGui::Text("Root Motion (1 loop)");
		ImGui::Text("  Delta : %.2f, %.2f, %.2f", dx, dy, dz);
		ImGui::Text("  XZ    : %.2f m / %.2f s  (%.2f m/s)", horizontal, anim.secondsLength, speed);
		return;
	}
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
