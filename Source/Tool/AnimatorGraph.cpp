#include "AnimatorGraph.h"
#include <algorithm>

namespace
{
	// enumの文字変換
	std::string ToString(AnimStateType type)
	{
		switch (type)
		{
		case AnimStateType::Entry: return "Entry";
		case AnimStateType::State: return "State";
		}
		return "State";
	}

	AnimStateType ToAnimStateType(const std::string& s)
	{
		if (s == "Entry") 
		{
			return AnimStateType::Entry;
		}
		return AnimStateType::State;
	}

	std::string ToString(AnimNodeType type)
	{
		switch (type)
		{
		case AnimNodeType::Normal: return "Normal";
		case AnimNodeType::Start:  return "Start";
		case AnimNodeType::End:    return "End";
		}
		return "Normal";
	}

	AnimNodeType ToAnimNodeType(const std::string& s)
	{
		if (s == "Start") return AnimNodeType::Start;
		if (s == "End")   return AnimNodeType::End;
		return AnimNodeType::Normal;
	}
}


int AnimatorGraph::CreateState(const std::string& name, float posX, float posY)
{
	AnimState state;
	state.id = GenerateId();
	state.name = name;
	state.posX = posX;
	state.posY = posY;
	state.type = AnimStateType::State;

	states.push_back(state);
	return state.id;
}

void AnimatorGraph::DeleteState(int stateId)
{
	states.erase(
		std::remove_if(states.begin(), states.end(),
			[stateId](const AnimState& s) { return s.id == stateId; }),
		states.end()
	);
}

// グラフ内容をJsonに変換する
nlohmann::ordered_json AnimatorGraph::ToJson() const
{
	nlohmann::ordered_json j;

	j["states"] = nlohmann::json::array();
	for (const auto& state : states)
	{
		nlohmann::ordered_json js;
		js["id"] = state.id;
		js["name"] = state.name;
		js["posX"] = state.posX;
		js["posY"] = state.posY;
		js["type"] = ToString(state.type);

		js["children"] = nlohmann::json::array();
		for (const auto& node : state.children)
		{
			nlohmann::ordered_json jn;
			jn["id"] = node.id;
			jn["name"] = node.name;
			jn["posX"] = node.posX;
			jn["posY"] = node.posY;
			jn["type"] = ToString(node.type);
			jn["clipName"] = node.clipName;
			js["children"].push_back(jn);
		}

		js["childLinks"] = nlohmann::json::array();
		for (const auto& link : state.childLinks)
		{
			nlohmann::ordered_json jl;
			jl["id"] = link.id;
			jl["fromId"] = link.fromId;
			jl["toId"] = link.toId;
			js["childLinks"].push_back(jl);
		}

		j["states"].push_back(js);
	}

	j["stateLinks"] = nlohmann::json::array();
	for (const auto& link : stateLinks)
	{
		nlohmann::ordered_json jl;
		jl["id"] = link.id;
		jl["fromId"] = link.fromId;
		jl["toId"] = link.toId;
		j["stateLinks"].push_back(jl);
	}

	return j;
}

// Jsonから中身を変換する
void AnimatorGraph::FromJson(const nlohmann::json& j)
{
	states.clear();
	stateLinks.clear();

	if (j.contains("states"))
	{
		for (const auto& js : j["states"])
		{
			AnimState state;
			state.id = js.value("id", -1);
			state.name = js.value("name", "");
			state.posX = js.value("posX", 0.0f);
			state.posY = js.value("posY", 0.0f);
			state.type = ToAnimStateType(js.value("type", "State"));

			if (js.contains("children"))
			{
				for (const auto& jn : js["children"])
				{
					AnimNode node;
					node.id = jn.value("id", -1);
					node.name = jn.value("name", "");
					node.posX = jn.value("posX", 0.0f);
					node.posY = jn.value("posY", 0.0f);
					node.type = ToAnimNodeType(jn.value("type", "Normal"));
					node.clipName = jn.value("clipName", "");
					state.children.push_back(node);
				}
			}

			if (js.contains("childLinks"))
			{
				for (const auto& jl : js["childLinks"])
				{
					AnimLink link;
					link.id = jl.value("id", -1);
					link.fromId = jl.value("fromId", -1);
					link.toId = jl.value("toId", -1);
					state.childLinks.push_back(link);
				}
			}

			states.push_back(state);

			// 復元後もIDが重複しないよう、採番カウンタを追従させる
			if (state.id >= nextId) nextId = state.id + 1;
		}
	}

	if (j.contains("stateLinks"))
	{
		for (const auto& jl : j["stateLinks"])
		{
			AnimLink link;
			link.id = jl.value("id", -1);
			link.fromId = jl.value("fromId", -1);
			link.toId = jl.value("toId", -1);
			stateLinks.push_back(link);

			if (link.id >= nextId) nextId = link.id + 1;
		}
	}
}
