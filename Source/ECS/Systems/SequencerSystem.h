#pragma once
#include "entt.hpp"
#include <string>
#include <unordered_map>
#include "Tool/SequencerData.h"

class SequencerSystem
{
public:
	void Update(entt::registry& registry);

private:
	// ファイルパスごとに、読み込み済みのSequencerDataをキャッシュする
	std::unordered_map<std::string, SequencerData> cache;

	// パスから、対応するSequencerDataを取得する(未読み込みなら、その場で読み込む)
	SequencerData& GetOrLoadSequencerData(const std::string& sequencerPath);

	// 1体のエンティティについて、フラグを更新する
	void UpdateFlags(SequencerComponent& seq, const SequencerClipData* clip, float normalizedTime);
};