#pragma once
#include <set>
#include <string>
#include "entt.hpp"
#include "Engine/Data/AnimationSpeedData.h"
#include "Engine/Core/JsonFileCache.h"

struct AnimatorComponent;

// 速度カーブから、アニメーションの再生速度の倍率を決める
class AnimationSpeedSystem
{
public:
	void Update(entt::registry& registry);

private:
	// Blend の速度カーブの倍率を求める
	float EvaluateBlendSpeed(const AnimationSpeedData& data, const AnimatorComponent& animator);

	JsonFileCache<AnimationSpeedData> dataCache;

	// 「材料のカーブかトリミングが違う」と報告済みの組(材料A + 材料B)
	std::set<std::string> reportedMismatch;
};