#pragma once
#include "entt.hpp"
#include "Data/SequencerData.h"
#include "Core/JsonFileCache.h"
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