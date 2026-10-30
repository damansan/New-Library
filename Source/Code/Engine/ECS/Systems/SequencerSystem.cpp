#include "SequencerSystem.h"
#include "Engine/ECS/Components/SkinnedMeshComponent.h"
#include "Engine/ECS/Components/AnimatorComponent.h"
#include <fstream>

// 1体のエンティティについて、フラグを更新する
void SequencerSystem::UpdateFlags(SequencerComponent& seq, const SequencerClipData* clip, float rawTime) 
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
			bool inRange = (rawTime >= r.start && rawTime <= r.end);
			setFlag(r.flag, inRange);
		}

		// Point:前フレームから今フレームの間を、この時刻が通過していればtrue
		for (const auto& p : clip->points)
		{
			bool passed = (seq.previousRawTime < p.time && rawTime >= p.time);
			setFlag(p.flag, passed);
		}
	}

	// 今回の時刻を、次のフレームのために保存しておく
	seq.previousRawTime = rawTime;
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

		// 焼き込み後のクリップを探す
		const Model::Animation* animation = nullptr;
		for (const auto& anim : skinned.model->GetAnimations())
		{
			if (anim.name == animator.currentAnimation)
			{
				animation = &anim;
				break;
			}
		}

		if (!animation)
		{
			continue;
		}

		// 今の再生時刻を、生データの時刻に直す
		const float rawTime = animation->ToRawTime(animator.animationTime);

		UpdateFlags(seq, clip, rawTime);
	}
}