#include "AnimationSpeedSystem.h"
#include <algorithm>
#include <cmath>
#include "Engine/Core/Log.h"
#include "Engine/ECS/Components/SkinnedMeshComponent.h"
#include "Engine/ECS/Components/AnimatorComponent.h"
#include "Engine/ECS/Components/AnimationSpeedComponent.h"

namespace
{
	const float sameEpsilon = 0.0001f; // 同じとみなす誤差

	// クリップ名から速度カーブを探す(無ければ nullptr。作らない)
	const AnimationSpeedClipData* FindSpeedClip(const AnimationSpeedData& data, const std::string& clipName)
	{
		for (const auto& c : data.clips)
		{
			if (c.clipName == clipName) return &c;
		}
		return nullptr;
	}

	// カーブがあるか(点が1つ以上)
	bool HasCurve(const AnimationSpeedClipData* curve)
	{
		return curve && !curve->points.empty();
	}

	// 2つのカーブの点が同じか(ID と並び順は見ない)
	bool SameCurve(const AnimationSpeedClipData& a, const AnimationSpeedClipData& b)
	{
		if (a.points.size() != b.points.size()) return false;

		auto byTime = [](const AnimationSpeedPoint& l, const AnimationSpeedPoint& r) { return l.time < r.time; };
		std::vector<AnimationSpeedPoint> sortedA = a.points;
		std::vector<AnimationSpeedPoint> sortedB = b.points;
		std::sort(sortedA.begin(), sortedA.end(), byTime);
		std::sort(sortedB.begin(), sortedB.end(), byTime);

		for (size_t i = 0; i < sortedA.size(); ++i)
		{
			if (std::abs(sortedA[i].time - sortedB[i].time) > sameEpsilon) return false;
			if (std::abs(sortedA[i].speed - sortedB[i].speed) > sameEpsilon) return false;
		}
		return true;
	}
}

void AnimationSpeedSystem::Update(entt::registry& registry)
{
	auto view = registry.view<SkinnedMeshComponent, AnimatorComponent, AnimationSpeedComponent>();

	for (auto entity : view)
	{
		auto& skinned = view.get<SkinnedMeshComponent>(entity);
		auto& animator = view.get<AnimatorComponent>(entity);
		auto& animSpeed = view.get<AnimationSpeedComponent>(entity);

		// パスが未設定なら、このエンティティは速度カーブを使わない
		if (animSpeed.animationSpeedPath.empty())
		{
			animator.speedCurveMultiplier = 1.0f;
			continue;
		}

		if (!skinned.model)
		{
			continue;
		}

		AnimationSpeedData& data = dataCache.GetOrLoad(animSpeed.animationSpeedPath,
			[this](AnimationSpeedData& data, const nlohmann::json& j)
			{
				data.FromJson(j);
				reportedMismatch.clear(); // 読み込み直したら、確かめ直せるようにする
			});

		// Blend：材料の速度カーブから決める
		if (animator.playMode == AnimPlayMode::Blend)
		{
			animator.speedCurveMultiplier = EvaluateBlendSpeed(data, animator);
			continue;
		}

		// Clip：今までどおり
		const AnimationSpeedClipData* clip = nullptr;
		for (const auto& c : data.clips)
		{
			if (c.clipName == animator.currentAnimation) { clip = &c; break; }
		}

		// クリップの長さを取得する
		float clipLength = 0.0f;
		for (const auto& anim : skinned.model->GetAnimations())
		{
			if (anim.name == animator.currentAnimation)
			{
				clipLength = anim.secondsLength;
				break;
			}
		}

		if (clipLength <= 0.0001f)
		{
			animator.speedCurveMultiplier = 1.0f;
			continue;
		}

		float normalizedTime = animator.animationTime / clipLength;

		animator.speedCurveMultiplier = clip ? clip->EvaluateSpeed(normalizedTime) : 1.0f;
	}
}

// Blend の速度カーブの倍率を求める
float AnimationSpeedSystem::EvaluateBlendSpeed(const AnimationSpeedData& data, const AnimatorComponent& animator)
{
	// A の「クリップ全体を 0?1 とした位置」(B もカーブとトリミングが同じときだけ使うので、A の位置で足りる)
	const float normalizedTime = animator.blendTrimStartA
		+ animator.blendPhase * (animator.blendTrimEndA - animator.blendTrimStartA);

	const AnimationSpeedClipData* curveA = FindSpeedClip(data, animator.blendClipA);

	// 材料1つ：A のカーブを使う
	if (animator.blendClipB.empty())
	{
		return HasCurve(curveA) ? curveA->EvaluateSpeed(normalizedTime) : 1.0f;
	}

	const AnimationSpeedClipData* curveB = FindSpeedClip(data, animator.blendClipB);

	// 2つともカーブが無い：元々カーブを使っていない
	if (!HasCurve(curveA) && !HasCurve(curveB)) return 1.0f;

	// カーブとトリミングが同じなら、そのカーブを使う
	const bool sameTrim =
		std::abs(animator.blendTrimStartA - animator.blendTrimStartB) <= sameEpsilon &&
		std::abs(animator.blendTrimEndA - animator.blendTrimEndB) <= sameEpsilon;
	const bool sameCurve = HasCurve(curveA) && HasCurve(curveB) && SameCurve(*curveA, *curveB);

	if (sameTrim && sameCurve)
	{
		return curveA->EvaluateSpeed(normalizedTime);
	}

	// 違う：両方 1倍で再生する
	std::string key = animator.blendClipA + "\n" + animator.blendClipB;
	if (reportedMismatch.insert(key).second)
	{
		Log(u8"[AnimationSpeed] 警告: Blend の材料「%s」と「%s」は、速度カーブかトリミングが違うため、速度カーブを使いません(1倍で再生します)",
			animator.blendClipA.c_str(), animator.blendClipB.c_str());
	}
	return 1.0f;
}
