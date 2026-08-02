#include "AnimatorGraph.h"
#include <algorithm>

// == enumの文字変換 == //
namespace
{
	// ===== enumの文字変換(Stateグラフ関連) =====

	// ChildStateType(子階層の種類)
	std::string ToString(ChildStateType type)
	{
		switch (type)
		{
		case ChildStateType::Normal: return "Normal";
		case ChildStateType::Start:  return "Start";
		case ChildStateType::End:    return "End";
		}
		return "Normal";
	}
	ChildStateType ToChildStateType(const std::string& s)
	{
		if (s == "Start") return ChildStateType::Start;
		if (s == "End")   return ChildStateType::End;
		return ChildStateType::Normal;
	}


	// ParentStateType(親階層の種類)
	std::string ToString(ParentStateType type)
	{
		switch (type)
		{
		case ParentStateType::Entry: return "Entry";
		case ParentStateType::State: return "State";
		}
		return "State";
	}
	ParentStateType ToParentStateType(const std::string& s)
	{
		if (s == "Entry")
		{
			return ParentStateType::Entry;
		}
		return ParentStateType::State;
	}


	// ===== enumの文字変換(条件グラフ関連) =====

	// ValueType(変数ノードが持つ値の型)
	std::string ToString(ValueType type)
	{
		switch (type)
		{
		case ValueType::Int:    return "Int";
		case ValueType::Float:  return "Float";
		case ValueType::Bool:   return "Bool";

		case ValueType::Float2: return "Float2";
		case ValueType::Float3: return "Float3";
		case ValueType::Float4: return "Float4";
		default:                return "Float";
		}
	}
	ValueType ToValueType(const std::string& s)
	{
		if (s == "Int")    return ValueType::Int;
		if (s == "Bool")   return ValueType::Bool;
		if (s == "Float2") return ValueType::Float2;
		if (s == "Float3") return ValueType::Float3;
		if (s == "Float4") return ValueType::Float4;
		return ValueType::Float;
	}


	// FloatComponent(構造体のどの成分を取り出すか)
	std::string ToString(FloatComponent c)
	{
		switch (c)
		{
		case FloatComponent::X: return "X";
		case FloatComponent::Y: return "Y";
		case FloatComponent::Z: return "Z";
		case FloatComponent::W: return "W";
		default:                return "None";
		}
	}
	FloatComponent ToFloatComponent(const std::string& s)
	{
		if (s == "X") return FloatComponent::X;
		if (s == "Y") return FloatComponent::Y;
		if (s == "Z") return FloatComponent::Z;
		if (s == "W") return FloatComponent::W;
		return FloatComponent::None;
	}

	// CompareOp(比較演算子の種類)
	std::string ToString(CompareOp op)
	{
		switch (op)
		{
		case CompareOp::Greater:      return "Greater";
		case CompareOp::Less:         return "Less";
		case CompareOp::Equal:        return "Equal";
		case CompareOp::NotEqual:     return "NotEqual";
		case CompareOp::GreaterEqual: return "GreaterEqual";
		case CompareOp::LessEqual:    return "LessEqual";
		default:                      return "Greater";
		}
	}
	CompareOp ToCompareOp(const std::string& s)
	{
		if (s == "Less")         return CompareOp::Less;
		if (s == "Equal")        return CompareOp::Equal;
		if (s == "NotEqual")     return CompareOp::NotEqual;
		if (s == "GreaterEqual") return CompareOp::GreaterEqual;
		if (s == "LessEqual")    return CompareOp::LessEqual;
		return CompareOp::Greater;
	}
}

// コンストラクタ
AnimatorGraph::AnimatorGraph()
{
	// 空のグラフを作る場合でも、Entryだけは必ず存在する状態にする
	EnsureEntryExists();
}

// Stateを作成する
int AnimatorGraph::CreateState(const std::string& name, float posX, float posY)
{
	ParentState state;
	state.id = GenerateId();
	state.name = name;
	state.posX = posX;
	state.posY = posY;
	state.type = ParentStateType::State;

	states.push_back(state);

	// 作製したStateのIDを返す
	return state.id;
}

// Stateを削除する
void AnimatorGraph::DeleteState(int stateId)
{
	states.erase(
		std::remove_if(states.begin(), states.end(),
			[stateId](const ParentState& s)
			{
				// Entryは削除させない
				if (s.type == ParentStateType::Entry) return false;
				return s.id == stateId;
			}),
		states.end()
	);

	// このStateに繋がっていたリンクも一緒に削除する
	stateLinks.erase(
		std::remove_if(stateLinks.begin(), stateLinks.end(),
			[stateId](const AnimLink& l)
			{
				return l.fromId == stateId || l.toId == stateId;
			}),
		stateLinks.end()
	);
}

// Linkを作成する
int AnimatorGraph::CreateLink(int fromId, int toId)
{
	AnimLink link;
	link.id = GenerateId();
	link.fromId = fromId;
	link.toId = toId;

	// このLink専用の条件グラフに、固定のReturnノードを1つ配置しておく
	link.condition.returnNode.id = link.condition.GenerateId();
	link.condition.returnNode.posX = 500.0f;
	link.condition.returnNode.posY = 200.0f;

	stateLinks.push_back(link);
	return link.id;
}
// Linkを削除する
void AnimatorGraph::DeleteLink(int linkId)
{
	stateLinks.erase(
		std::remove_if(stateLinks.begin(), stateLinks.end(),
			[linkId](const AnimLink& l) { return l.id == linkId; }),
		stateLinks.end()
	);
}

// グラフ内容をJsonに変換する
nlohmann::ordered_json AnimatorGraph::ToJson() const
{
	nlohmann::ordered_json j;
	j["states"] = StatesToJson();
	j["stateLinks"] = StateLinksToJson();
	return j;
}

// Jsonから中身を変換する
void AnimatorGraph::FromJson(const nlohmann::json& j)
{
	states.clear();
	stateLinks.clear();

	if (j.contains("states")) StatesFromJson(j["states"]);
	if (j.contains("stateLinks")) StateLinksFromJson(j["stateLinks"]);

	// 読み込んだ結果Entryが無かった場合の保険
	EnsureEntryExists();
}

// Entryがなければ作成する
void AnimatorGraph::EnsureEntryExists()
{
	bool hasEntry = false;
	for (const auto& s : states)
	{
		if (s.type == ParentStateType::Entry)
		{
			hasEntry = true;
			break;
		}
	}

	if (!hasEntry)
	{
		ParentState entry;
		entry.id = GenerateId();
		entry.name = "Entry";
		entry.posX = 50.0f;
		entry.posY = 200.0f;
		entry.type = ParentStateType::Entry;
		states.push_back(entry);
	}
}

// Statesを保存内容へ変換
nlohmann::ordered_json AnimatorGraph::StatesToJson() const
{
	nlohmann::ordered_json jStates = nlohmann::json::array();

	for (const auto& state : states)
	{
		nlohmann::ordered_json js;
		js["id"] = state.id;
		js["name"] = state.name;
		js["posX"] = state.posX;
		js["posY"] = state.posY;
		js["type"] = ToString(state.type);
		js["color"] = { state.color[0], state.color[1], state.color[2] };

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

		jStates.push_back(js);
	}

	return jStates;
}
// Statesを保存内容から復元する
void AnimatorGraph::StatesFromJson(const nlohmann::json& j)
{
	for (const auto& js : j)
	{
		ParentState state;
		state.id = js.value("id", -1);
		state.name = js.value("name", "");
		state.posX = js.value("posX", 0.0f);
		state.posY = js.value("posY", 0.0f);
		state.type = ToParentStateType(js.value("type", "State"));

		if (js.contains("color") && js["color"].is_array() && js["color"].size() == 3)
		{
			state.color[0] = js["color"][0].get<float>();
			state.color[1] = js["color"][1].get<float>();
			state.color[2] = js["color"][2].get<float>();
		}

		if (js.contains("children"))
		{
			for (const auto& jn : js["children"])
			{
				ChildState node;
				node.id = jn.value("id", -1);
				node.name = jn.value("name", "");
				node.posX = jn.value("posX", 0.0f);
				node.posY = jn.value("posY", 0.0f);
				node.type = ToChildStateType(jn.value("type", "Normal"));
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

// StateLinksを保存内容へ変換
nlohmann::ordered_json AnimatorGraph::StateLinksToJson() const
{
	nlohmann::ordered_json jLinks = nlohmann::json::array();

	for (const auto& link : stateLinks)
	{
		nlohmann::ordered_json jl;
		jl["id"] = link.id;
		jl["fromId"] = link.fromId;
		jl["toId"] = link.toId;

		nlohmann::ordered_json jc;
		jc["nextId"] = link.condition.nextId;

		jc["returnNode"]["id"] = link.condition.returnNode.id;
		jc["returnNode"]["posX"] = link.condition.returnNode.posX;
		jc["returnNode"]["posY"] = link.condition.returnNode.posY;

		jc["variableNodes"] = nlohmann::json::array();
		for (const auto& v : link.condition.variableNodes)
		{
			nlohmann::ordered_json jv;
			jv["id"] = v.id;
			jv["posX"] = v.posX;
			jv["posY"] = v.posY;
			jv["name"] = v.name;
			jv["type"] = ToString(v.type);
			jv["intValue"] = v.intValue;
			jv["floatValue"] = v.floatValue;
			jv["boolValue"] = v.boolValue;
			jc["variableNodes"].push_back(jv);
		}

		jc["operatorNodes"] = nlohmann::json::array();
		for (const auto& o : link.condition.operatorNodes)
		{
			nlohmann::ordered_json jo;
			jo["id"] = o.id;
			jo["posX"] = o.posX;
			jo["posY"] = o.posY;
			jo["op"] = ToString(o.op);
			jc["operatorNodes"].push_back(jo);
		}

		jc["getterNodes"] = nlohmann::json::array();
		for (const auto& g : link.condition.getterNodes)
		{
			nlohmann::ordered_json jg;
			jg["id"] = g.id;
			jg["posX"] = g.posX;
			jg["posY"] = g.posY;
			jg["componentName"] = g.componentName;
			jg["fieldName"] = g.fieldName;
			jg["type"] = ToString(g.type);
			jg["selectedComponent"] = ToString(g.selectedComponent);
			jc["getterNodes"].push_back(jg);
		}

		jc["links"] = nlohmann::json::array();
		for (const auto& cl : link.condition.links)
		{
			nlohmann::ordered_json jcl;
			jcl["id"] = cl.id;
			jcl["fromId"] = cl.fromId;
			jcl["toId"] = cl.toId;
			jcl["toPinB"] = cl.toPinB;
			jc["links"].push_back(jcl);
		}

		jl["condition"] = jc;

		jLinks.push_back(jl);
	}

	return jLinks;
}
// StateLinksを保存内容から復元する
void AnimatorGraph::StateLinksFromJson(const nlohmann::json& j)
{
	for (const auto& jl : j)
	{
		AnimLink link;
		link.id = jl.value("id", -1);
		link.fromId = jl.value("fromId", -1);
		link.toId = jl.value("toId", -1);

		if (jl.contains("condition"))
		{
			const auto& jc = jl["condition"];
			link.condition.nextId = jc.value("nextId", 1);

			if (jc.contains("returnNode"))
			{
				link.condition.returnNode.id = jc["returnNode"].value("id", -1);
				link.condition.returnNode.posX = jc["returnNode"].value("posX", 500.0f);
				link.condition.returnNode.posY = jc["returnNode"].value("posY", 200.0f);
			}

			// 古い保存データにReturnノードが無かった場合の保険
			if (link.condition.returnNode.id < 0)
			{
				link.condition.returnNode.id = link.condition.GenerateId();
				link.condition.returnNode.posX = 500.0f;
				link.condition.returnNode.posY = 200.0f;
			}

			if (jc.contains("variableNodes"))
			{
				for (const auto& jv : jc["variableNodes"])
				{
					ConditionVariableNode v;
					v.id = jv.value("id", -1);
					v.posX = jv.value("posX", 0.0f);
					v.posY = jv.value("posY", 0.0f);
					v.name = jv.value("name", "");
					v.type = ToValueType(jv.value("type", "Float"));
					v.intValue = jv.value("intValue", 0);
					v.floatValue = jv.value("floatValue", 0.0f);
					v.boolValue = jv.value("boolValue", false);
					link.condition.variableNodes.push_back(v);
				}
			}

			if (jc.contains("operatorNodes"))
			{
				for (const auto& jo : jc["operatorNodes"])
				{
					ConditionOperatorNode o;
					o.id = jo.value("id", -1);
					o.posX = jo.value("posX", 0.0f);
					o.posY = jo.value("posY", 0.0f);
					o.op = ToCompareOp(jo.value("op", "Greater"));
					link.condition.operatorNodes.push_back(o);
				}
			}

			if (jc.contains("getterNodes"))
			{
				for (const auto& jg : jc["getterNodes"])
				{
					ConditionGetterNode g;
					g.id = jg.value("id", -1);
					g.posX = jg.value("posX", 0.0f);
					g.posY = jg.value("posY", 0.0f);
					g.componentName = jg.value("componentName", "");
					g.fieldName = jg.value("fieldName", "");
					g.type = ToValueType(jg.value("type", "Float"));
					g.selectedComponent = ToFloatComponent(jg.value("selectedComponent", "None"));
					link.condition.getterNodes.push_back(g);
				}
			}

			if (jc.contains("links"))
			{
				for (const auto& jcl : jc["links"])
				{
					ConditionLink cl;
					cl.id = jcl.value("id", -1);
					cl.fromId = jcl.value("fromId", -1);
					cl.toId = jcl.value("toId", -1);
					cl.toPinB = jcl.value("toPinB", false);
					link.condition.links.push_back(cl);
				}
			}
		}

		stateLinks.push_back(link);

		if (link.id >= nextId) nextId = link.id + 1;
	}
}
