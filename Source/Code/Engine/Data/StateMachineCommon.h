#pragma once
#include <vector>
#include <json.hpp>
#include "Engine/Data/ConditionGraphData.h"

// ステートマシンのデータ(StateMachineData / StateMachineBlendData)で共通して使う部品

// エディタ上の位置
struct EditorPosition
{
	float x = 0.0f;
	float y = 0.0f;
};

namespace StateMachineCommon
{
	// エディタ上の位置の保存と読み込み
	inline nlohmann::ordered_json PositionToJson(const EditorPosition& p)
	{
		nlohmann::ordered_json j;
		j["x"] = p.x;
		j["y"] = p.y;
		return j;
	}
	inline EditorPosition PositionFromJson(const nlohmann::json& j)
	{
		EditorPosition p;
		p.x = j.value("x", 0.0f);
		p.y = j.value("y", 0.0f);
		return p;
	}

	// 遷移・つながりの共通部分(always / condition)の保存と読み込み
	inline void WriteCondition(nlohmann::ordered_json& j, bool always, const ConditionGraph& condition)
	{
		j["always"] = always;
		j["condition"] = ConditionGraphToJson(condition);
	}
	inline void ReadCondition(const nlohmann::json& j, bool& always, ConditionGraph& condition)
	{
		always = j.value("always", false);
		if (j.contains("condition")) ConditionGraphFromJson(condition, j["condition"]);
	}

	// IDで要素を探す
	template<typename T>
	const T* FindById(const std::vector<T>& list, int id)
	{
		for (const auto& e : list)
		{
			if (e.id == id) return &e;
		}
		return nullptr;
	}
}