#include "ConditionGraphData.h"
#include <algorithm>

namespace
{
	// ValueType
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

	// FloatComponent
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

	// CompareOp
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

	// LogicOp
	std::string ToString(LogicOp op)
	{
		switch (op)
		{
		case LogicOp::And: return "And";
		case LogicOp::Or:  return "Or";
		default:           return "And";
		}
	}
	LogicOp ToLogicOp(const std::string& s)
	{
		if (s == "Or") return LogicOp::Or;
		return LogicOp::And;
	}
}

// 使用中のIDの最大値+1に、採番をそろえる
void ConditionGraph::RecalculateNextId()
{
	int maxId = returnNode.id;
	for (const auto& n : variableNodes) maxId = (std::max)(maxId, n.id);
	for (const auto& n : getterNodes)   maxId = (std::max)(maxId, n.id);
	for (const auto& n : operatorNodes) maxId = (std::max)(maxId, n.id);
	for (const auto& n : logicNodes)    maxId = (std::max)(maxId, n.id);
	for (const auto& n : notNodes)      maxId = (std::max)(maxId, n.id);
	for (const auto& l : links)         maxId = (std::max)(maxId, l.id);
	nextId = maxId + 1;
}

// Returnに何もつながっていないか
bool ConditionGraph::IsEmpty() const
{
	for (const auto& l : links)
	{
		if (l.toId == returnNode.id) return false;
	}
	return true;
}

// JSONへ変換
nlohmann::ordered_json ConditionGraphToJson(const ConditionGraph& graph)
{
	nlohmann::ordered_json jc;
	jc["returnNode"]["id"] = graph.returnNode.id;
	jc["returnNode"]["posX"] = graph.returnNode.posX;
	jc["returnNode"]["posY"] = graph.returnNode.posY;

	jc["variableNodes"] = nlohmann::json::array();
	for (const auto& v : graph.variableNodes)
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
	for (const auto& o : graph.operatorNodes)
	{
		nlohmann::ordered_json jo;
		jo["id"] = o.id;
		jo["posX"] = o.posX;
		jo["posY"] = o.posY;
		jo["op"] = ToString(o.op);
		jc["operatorNodes"].push_back(jo);
	}

	jc["logicNodes"] = nlohmann::json::array();
	for (const auto& l : graph.logicNodes)
	{
		nlohmann::ordered_json jl;
		jl["id"] = l.id;
		jl["posX"] = l.posX;
		jl["posY"] = l.posY;
		jl["op"] = ToString(l.op);
		jc["logicNodes"].push_back(jl);
	}

	jc["notNodes"] = nlohmann::json::array();
	for (const auto& n : graph.notNodes)
	{
		nlohmann::ordered_json jn;
		jn["id"] = n.id;
		jn["posX"] = n.posX;
		jn["posY"] = n.posY;
		jc["notNodes"].push_back(jn);
	}

	jc["getterNodes"] = nlohmann::json::array();
	for (const auto& g : graph.getterNodes)
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
	for (const auto& cl : graph.links)
	{
		nlohmann::ordered_json jcl;
		jcl["id"] = cl.id;
		jcl["fromId"] = cl.fromId;
		jcl["toId"] = cl.toId;
		jcl["toPinB"] = cl.toPinB;
		jc["links"].push_back(jcl);
	}

	return jc;
}

// JSONから復元
void ConditionGraphFromJson(ConditionGraph& graph, const nlohmann::json& jc)
{
	graph = ConditionGraph{};

	if (jc.contains("returnNode"))
	{
		graph.returnNode.id = jc["returnNode"].value("id", 1);
		graph.returnNode.posX = jc["returnNode"].value("posX", 500.0f);
		graph.returnNode.posY = jc["returnNode"].value("posY", 200.0f);
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
			graph.variableNodes.push_back(v);
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
			graph.operatorNodes.push_back(o);
		}
	}

	if (jc.contains("logicNodes"))
	{
		for (const auto& jl : jc["logicNodes"])
		{
			ConditionLogicNode l;
			l.id = jl.value("id", -1);
			l.posX = jl.value("posX", 0.0f);
			l.posY = jl.value("posY", 0.0f);
			l.op = ToLogicOp(jl.value("op", "And"));
			graph.logicNodes.push_back(l);
		}
	}

	if (jc.contains("notNodes"))
	{
		for (const auto& jn : jc["notNodes"])
		{
			ConditionNotNode n;
			n.id = jn.value("id", -1);
			n.posX = jn.value("posX", 0.0f);
			n.posY = jn.value("posY", 0.0f);
			graph.notNodes.push_back(n);
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
			graph.getterNodes.push_back(g);
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
			graph.links.push_back(cl);
		}
	}

	// 読み込んだIDの続きから採番する
	graph.RecalculateNextId();
}