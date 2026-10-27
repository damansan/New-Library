#include "AnimationEditData.h"
#include <filesystem>

// 指定したクリップ名のデータを探す
const AnimationEditClipData* AnimationEditData::FindClip(const std::string& clipName) const
{
	for (const auto& clip : clips)
	{
		if (clip.clipName == clipName) return &clip;
	}
	return nullptr;
}

// 指定したクリップ名のデータを探す
AnimationEditClipData* AnimationEditData::FindOrCreateClip(const std::string& clipName)
{
	for (auto& clip : clips)
	{
		if (clip.clipName == clipName) return &clip;
	}

	AnimationEditClipData newClip;
	newClip.clipName = clipName;
	clips.push_back(newClip);
	return &clips.back();
}

// 保存
nlohmann::ordered_json AnimationEditData::ToJson() const
{
	nlohmann::ordered_json j;
	j["rootNodeName"] = rootNodeName;
	j["clips"] = nlohmann::json::array();

	for (const auto& clip : clips)
	{
		nlohmann::ordered_json jc;
		jc["clipName"] = clip.clipName;
		jc["yawOffsetDegrees"] =  clip.yawOffsetDegrees;
		jc["rootMotionScaleXZ"] = clip.rootMotionScaleXZ;
		jc["rootMotionScaleY"] =  clip.rootMotionScaleY;
		j["clips"].push_back(jc);
	}

	return j;
}

// 読み込み
void AnimationEditData::FromJson(const nlohmann::json& j)
{
	rootNodeName = j.value("rootNodeName", "");
	clips.clear();

	if (!j.contains("clips")) return;

	for (const auto& jc : j["clips"])
	{
		AnimationEditClipData clip;
		clip.clipName = jc.value("clipName", "");
		clip.yawOffsetDegrees =  jc.value("yawOffsetDegrees",  0.0f);
		clip.rootMotionScaleXZ = jc.value("rootMotionScaleXZ", 1.0f);
		clip.rootMotionScaleY =  jc.value("rootMotionScaleY",  1.0f);
		clips.push_back(clip);
	}
}

// モデルパスから保存先のパスを組み立てる
std::string AnimationEditData::GetFilePath(const std::string& modelPath)
{
	std::filesystem::path p(modelPath);
	return "Data/Json/Animation/" + p.stem().string() + ".animedit";
}