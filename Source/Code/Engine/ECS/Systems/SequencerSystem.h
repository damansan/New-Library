#pragma once
#include "entt.hpp"
#include "Engine/Data/SequencerData.h"
#include "Engine/Core/JsonFileCache.h"
#include <string>
#include <unordered_map>

class SequencerSystem
{
public:
	void Update(entt::registry& registry);

private:
	JsonFileCache<SequencerData> dataCache;
	// 1体のエンティティについて、フラグを更新する
	void UpdateFlags(SequencerComponent& seq, const SequencerClipData* clip, float normalizedTime);
};