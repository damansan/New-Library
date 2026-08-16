#include "SequencerSystem.h"
#include "ECS/Components/SkinnedMeshComponent.h"
#include "ECS/Components/AnimatorComponent.h"
#include <fstream>

// 1体のエンティティについて、フラグを更新する
void SequencerSystem::UpdateFlags(SequencerComponent& seq, const SequencerClipData* clip, float normalizedTime)
{
	// フラグを一旦全部falseに戻す
	seq.animExit = false;
	seq.attack = false;

	if (clip)
	{
		auto setFlag = [&](SequencerFlag flag, bool value)
			{
				switch (flag)
				{
				case SequencerFlag::AnimExit: seq.animExit = seq.animExit || value; break;
				case SequencerFlag::Attack:   seq.attack = seq.attack || value; break;
				}
			};

		// Range:今の時刻が範囲内ならtrue
		for (const auto& r : clip->ranges)
		{
			bool inRange = (normalizedTime >= r.start && normalizedTime <= r.end);
			setFlag(r.flag, inRange);
		}

		// Point:前フレームから今フレームの間を、この時刻が通過していればtrue
		for (const auto& p : clip->points)
		{
			bool passed = (seq.previousNormalizedTime < p.time && normalizedTime >= p.time);
			setFlag(p.flag, passed);
		}
	}

	// 今回の時刻を、次のフレームのために保存しておく
	seq.previousNormalizedTime = normalizedTime;
}

void SequencerSystem::Update(entt::registry& registry)
{
	auto view = registry.view<SkinnedMeshComponent, AnimatorComponent, SequencerComponent>();

	for (auto entity : view)
	{
		auto& skinned = view.get<SkinnedMeshComponent>(entity);
		auto& animator = view.get<AnimatorComponent>(entity);
		auto& seq = view.get<SequencerComponent>(entity);

		// パスが未設定なら、このエンティティはSequencerを使わない
		if (seq.sequencerPath.empty())
		{
			continue;
		}

		if (!skinned.model)
		{
			continue;
		}

		SequencerData& data = dataCache.GetOrLoad(seq.sequencerPath,
			[](SequencerData& data, const nlohmann::json& j) { data.FromJson(j); });

		SequencerClipData* clip = nullptr;
		for (auto& c : data.clips)
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
			continue;
		}

		float normalizedTime = animator.animationTime / clipLength;

		UpdateFlags(seq, clip, normalizedTime);
	}
}