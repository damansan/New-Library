#include "AnimatorGraphData.h"
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


// 親階層のStateを作成する
int ParentStateGraph::CreateParentState(const std::string& name, float posX, float posY)
{
	ParentState state;
	state.id = GenerateId();
	state.name = name;
	state.posX = posX;
	state.posY = posY;
	state.type = ParentStateType::State;

	// 新しく作ったStateの子階層グラフに、案内板を配線する
	state.childGraph.idCounter = this->idCounter;
	// 配線が終わってから、Start/Endを保証する
	state.childGraph.EnsureStartAndEndExist();

	states.push_back(state);

	return state.id;
}
// 親階層のLinkを作成する
int ParentStateGraph::CreateParentLink(int fromId, int toId)
{
	AnimLink link;
	link.id = GenerateId();
	link.fromId = fromId;
	link.toId = toId;

	// 条件グラフにも案内板を配線してから使う
	link.condition.idCounter = this->idCounter;
	// このLink専用の条件グラフに、固定のReturnノードを1つ配置しておく
	link.condition.returnNode.id = link.condition.GenerateId();
	link.condition.returnNode.posX = 500.0f;
	link.condition.returnNode.posY = 200.0f;

	stateLinks.push_back(link);
	return link.id;
}

// 親階層のStateを削除する
void ParentStateGraph::DeleteParentState(int stateId)
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
// 親階層のLinkを削除する
void ParentStateGraph::DeleteParentLink(int linkId)
{
	stateLinks.erase(
		std::remove_if(stateLinks.begin(), stateLinks.end(),
			[linkId](const AnimLink& l) { return l.id == linkId; }),
		stateLinks.end()
	);
}

// Entryがなければ作成する
void ParentStateGraph::EnsureEntryExists()
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

		// Entryの子階層グラフにも案内板だけは配線しておく(Start/Endは保証しない)
		entry.childGraph.idCounter = this->idCounter;

		states.push_back(entry);
	}
}


// 子階層のStateを作成する
int ChildStateGraph::CreateChildState(const std::string& name, float posX, float posY)
{
	ChildState state;
	state.id = GenerateId();
	state.name = name;
	state.posX = posX;
	state.posY = posY;
	state.type = ChildStateType::Normal;

	children.push_back(state);

	return state.id;
}
// 子階層のLinkを作成する
int ChildStateGraph::CreateChildLink(int fromId, int toId)
{
	AnimLink link;
	link.id = GenerateId();
	link.fromId = fromId;
	link.toId = toId;

	// 条件グラフにも案内板を配線してから使う
	link.condition.idCounter = this->idCounter;
	// このLink専用の条件グラフに、固定のReturnノードを1つ配置しておく
	link.condition.returnNode.id = link.condition.GenerateId();
	link.condition.returnNode.posX = 500.0f;
	link.condition.returnNode.posY = 200.0f;

	childLinks.push_back(link);

	return link.id;
}

// 子階層のStateを削除する
void ChildStateGraph::DeleteChildState(int stateId)
{
	children.erase(
		std::remove_if(children.begin(), children.end(),
			[stateId](const ChildState& s)
			{
				// Start/Endは削除させない
				if (s.type == ChildStateType::Start || s.type == ChildStateType::End) return false;
				return s.id == stateId;
			}),
		children.end()
	);

	// このStateに繋がっていたLinkも一緒に削除する
	childLinks.erase(
		std::remove_if(childLinks.begin(), childLinks.end(),
			[stateId](const AnimLink& l)
			{
				return l.fromId == stateId || l.toId == stateId;
			}),
		childLinks.end()
	);
}
// 子階層のLinkを削除する
void ChildStateGraph::DeleteChildLink(int linkId)
{
	childLinks.erase(
		std::remove_if(childLinks.begin(), childLinks.end(),
			[linkId](const AnimLink& l) { return l.id == linkId; }),
		childLinks.end()
	);
}

// Start/Endがなければ作成する
void ChildStateGraph::EnsureStartAndEndExist()
{
	bool hasStart = false;
	bool hasEnd = false;

	for (const auto& child : children)
	{
		if (child.type == ChildStateType::Start) hasStart = true;
		if (child.type == ChildStateType::End)   hasEnd = true;
	}

	if (!hasStart)
	{
		CreateChildState("Entry", 50.0f, 200.0f);
		children.back().type = ChildStateType::Start;
	}

	if (!hasEnd)
	{
		CreateChildState("Exit", 400.0f, 200.0f);
		children.back().type = ChildStateType::End;
	}
}


// コンストラクタ
AnimatorGraphData::AnimatorGraphData()
{
	// idCounterの実体(this自身が持つ)を、parentGraphの案内板に配線する
	parentGraph.idCounter = &idCounter;

	// 配線が終わってから、Entryの保証を行う
	parentGraph.EnsureEntryExists();
}


// グラフ内容をJsonに変換する
nlohmann::ordered_json AnimatorGraphData::ToJson() const
{
	nlohmann::ordered_json j;
	j["states"] = StatesToJson();
	j["stateLinks"] = StateLinksToJson();
	return j;
}
// Jsonから中身を変換する
void AnimatorGraphData::FromJson(const nlohmann::json& j)
{
	parentGraph.states.clear();
	parentGraph.stateLinks.clear();

	if (j.contains("states")) StatesFromJson(j["states"]);
	if (j.contains("stateLinks")) StateLinksFromJson(j["stateLinks"]);

	// 読み込んだ結果Entryが無かった場合の保険
	parentGraph.EnsureEntryExists();
}


// Statesを保存内容へ変換
nlohmann::ordered_json AnimatorGraphData::StatesToJson() const
{
	nlohmann::ordered_json jStates = nlohmann::json::array();

	for (const auto& state : parentGraph.states)
	{
		nlohmann::ordered_json js;
		js["id"] = state.id;
		js["name"] = state.name;
		js["posX"] = state.posX;
		js["posY"] = state.posY;
		js["type"] = ToString(state.type);
		js["color"] = { state.color[0], state.color[1], state.color[2] };

		js["children"] = nlohmann::json::array();
		for (const auto& node : state.childGraph.children)
		{
			nlohmann::ordered_json jn;
			jn["id"] = node.id;
			jn["name"] = node.name;
			jn["posX"] = node.posX;
			jn["posY"] = node.posY;
			jn["type"] = ToString(node.type);
			jn["clipName"] = node.clipName;
			jn["loop"] = node.loop;
			jn["useRootMotion"] = node.useRootMotion;
			js["children"].push_back(jn);
		}

		js["childLinks"] = nlohmann::json::array();
		for (const auto& link : state.childGraph.childLinks)
		{
			js["childLinks"].push_back(AnimLinkToJson(link));
		}

		jStates.push_back(js);
	}

	return jStates;
}
// Statesを保存内容から復元する
void AnimatorGraphData::StatesFromJson(const nlohmann::json& j)
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

		// 子階層グラフに案内板を配線してから、JSONの内容を読み込む
		state.childGraph.idCounter = &idCounter;
		// state.childGraph のコンストラクタが自動生成したStart/Endを、
		// JSON読み込み前に一旦取り除いておく(二重生成防止)
		state.childGraph.children.clear();
		state.childGraph.childLinks.clear();

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
				node.loop = jn.value("loop", true);
				node.useRootMotion = jn.value("useRootMotion", false);
				state.childGraph.children.push_back(node);

				SyncIdCounter(node.id);
			}
		}

		if (js.contains("childLinks"))
		{
			for (const auto& jl : js["childLinks"])
			{
				AnimLink link;
				AnimLinkFromJson(link, jl);
				state.childGraph.childLinks.push_back(link);
			}
		}

		// 古い保存データに、子階層のStart/Endが無かった場合の保険
		state.childGraph.EnsureStartAndEndExist();

		SyncIdCounter(state.id);

		parentGraph.states.push_back(state);
	}
}

// StateLinksを保存内容へ変換
nlohmann::ordered_json AnimatorGraphData::StateLinksToJson() const
{
	nlohmann::ordered_json jLinks = nlohmann::json::array();

	for (const auto& link : parentGraph.stateLinks)
	{
		jLinks.push_back(AnimLinkToJson(link));
	}

	return jLinks;
}
// StateLinksを保存内容から復元する
void AnimatorGraphData::StateLinksFromJson(const nlohmann::json& j)
{
	for (const auto& jl : j)
	{
		AnimLink link;
		AnimLinkFromJson(link, jl);
		parentGraph.stateLinks.push_back(link);
	}
}


// AnimLink1本分をJsonに変換する
nlohmann::ordered_json AnimatorGraphData::AnimLinkToJson(const AnimLink& link) const
{
	nlohmann::ordered_json jl;
	jl["id"] = link.id;
	jl["fromId"] = link.fromId;
	jl["toId"] = link.toId;
	jl["blend"] = link.blend;
	jl["blendDuration"] = link.blendDuration;

	nlohmann::ordered_json jc;

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

	return jl;
}
// AnimLink1本分をJsonから復元する
void AnimatorGraphData::AnimLinkFromJson(AnimLink& link, const nlohmann::json& jl)
{
	link.id = jl.value("id", -1);
	link.fromId = jl.value("fromId", -1);
	link.toId = jl.value("toId", -1);
	link.blend = jl.value("blend", true);
	link.blendDuration = jl.value("blendDuration", 0.2f);

	// 条件グラフに案内板を配線してから使う
	link.condition.idCounter = &idCounter;

	if (jl.contains("condition"))
	{
		const auto& jc = jl["condition"];

		if (jc.contains("returnNode"))
		{
			link.condition.returnNode.id = jc["returnNode"].value("id", -1);
			link.condition.returnNode.posX = jc["returnNode"].value("posX", 500.0f);
			link.condition.returnNode.posY = jc["returnNode"].value("posY", 200.0f);
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

				SyncIdCounter(v.id);
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

				SyncIdCounter(o.id);
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

				SyncIdCounter(g.id);
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

				SyncIdCounter(cl.id);
			}
		}
	}

	// 古い保存データ、あるいは"condition"キー自体が無かった場合の保険
	if (link.condition.returnNode.id < 0)
	{
		link.condition.returnNode.id = link.condition.GenerateId();
		link.condition.returnNode.posX = 500.0f;
		link.condition.returnNode.posY = 200.0f;
	}
	SyncIdCounter(link.condition.returnNode.id);

	SyncIdCounter(link.id);
}

// 読み込んだIDにカウンタを追従させる
void AnimatorGraphData::SyncIdCounter(int id)
{
	if (id >= idCounter) idCounter = id + 1;
}
