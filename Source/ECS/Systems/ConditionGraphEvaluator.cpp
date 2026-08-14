#include "ConditionGraphEvaluator.h"
#include "Editor/ComponentRegistry.h"
#include "Editor/MetaRegistry.h"
#include <DirectXMath.h>
#include <cmath>

// 遷移条件が成立していればtrueを返す
bool ConditionGraphEvaluator::Evaluate(const ConditionGraph& graph, entt::registry& registry, entt::entity entity)
{
	int fromId = -1;
	for (const auto& link : graph.links)
	{
		if (link.toId == graph.returnNode.id)
		{
			fromId = link.fromId;
			break;
		}
	}
	if (fromId == -1) return true; // Returnに何も繋がっていない場合は、無条件遷移として扱う

	// 繋がっているのがOperatorなら、その比較結果をそのまま返す
	for (const auto& o : graph.operatorNodes)
		if (o.id == fromId) return EvaluateOperator(graph, o, registry, entity);
	// 繋がっているのがGetter(Bool型)なら、0.5f超えをtrueとみなす
	for (const auto& g : graph.getterNodes)
		if (g.id == fromId) return GetGetterValue(g, registry, entity) > 0.5f;
	// 繋がっているのがVariable(Bool型)なら、0.5f超えをtrueとみなす
	for (const auto& v : graph.variableNodes)
		if (v.id == fromId) return GetVariableValue(v) > 0.5f;

	return false; // どれにも該当しない場合
}

// ゲッターノードで取得している値を取得する
float ConditionGraphEvaluator::GetGetterValue(const ConditionGetterNode& node, entt::registry& registry, entt::entity entity)
{
	// コンポーネント名とフィールド名から一致するFieldEntryを探す
	const FieldEntry* found = nullptr;
	for (const auto& f : MetaRegistry::GetAllFields())
	{
		if (f.componentName == node.componentName && f.fieldLabel == node.fieldName)
		{
			found = &f;
			break;
		}
	}
	if (!found) return 0.0f;

	// コンポーネント名からエンティティの持つ
	const ComponentInfo* info = ComponentRegistry::GetByTypeName(node.componentName);

	if (!info) return 0.0f;

	entt::meta_any instance = info->getMeta(registry, entity);
	if (!instance) return 0.0f;

	// 型情報からフィードの値を取得する
	entt::meta_data data = found->componentType.data(found->fieldId);
	entt::meta_any value = data.get(instance);

	// 各型をfloatに変換する
	if (float* f = value.try_cast<float>()) return *f;
	if (int* i = value.try_cast<int>())     return static_cast<float>(*i);
	if (bool* b = value.try_cast<bool>())   return *b ? 1.0f : 0.0f;

	if (DirectX::XMFLOAT2* v = value.try_cast<DirectX::XMFLOAT2>())
	{
		switch (node.selectedComponent)
		{
		case FloatComponent::X: return v->x;
		case FloatComponent::Y: return v->y;
		default: return 0.0f;
		}
	}
	if (DirectX::XMFLOAT3* v = value.try_cast<DirectX::XMFLOAT3>())
	{
		switch (node.selectedComponent)
		{
		case FloatComponent::X: return v->x;
		case FloatComponent::Y: return v->y;
		case FloatComponent::Z: return v->z;
		default: return 0.0f;
		}
	}
	if (DirectX::XMFLOAT4* v = value.try_cast<DirectX::XMFLOAT4>())
	{
		switch (node.selectedComponent)
		{
		case FloatComponent::X: return v->x;
		case FloatComponent::Y: return v->y;
		case FloatComponent::Z: return v->z;
		case FloatComponent::W: return v->w;
		default: return 0.0f;
		}
	}

	return 0.0f;
}
// 変数ノードで保持している値を取得する
float ConditionGraphEvaluator::GetVariableValue(const ConditionVariableNode& node)
{
	switch (node.type)
	{
	case ValueType::Int:   return static_cast<float>(node.intValue);
	case ValueType::Float: return node.floatValue;
	case ValueType::Bool:  return node.boolValue ? 1.0f : 0.0f;
	default:               return 0.0f;
	}
}

// 比較ノードを演算し結果を返す
bool ConditionGraphEvaluator::EvaluateOperator(const ConditionGraph& graph, const ConditionOperatorNode& op, entt::registry& registry, entt::entity entity)
{
	// A/Bそれぞれに繋がっているノードのIDを探す
	int fromIdA = -1;
	int fromIdB = -1;
	for (const auto& link : graph.links)
	{
		if (link.toId != op.id) continue;
		if (link.toPinB) fromIdB = link.fromId;
		else             fromIdA = link.fromId;
	}

	if (fromIdA == -1 || fromIdB == -1) return false; // 片方でも未接続なら、不成立扱い

	float a = ResolveInputValue(graph, fromIdA, registry, entity);
	float b = ResolveInputValue(graph, fromIdB, registry, entity);

	const float epsilon = 0.0001f; // 浮動小数点の誤差を吸収するための許容範囲

	switch (op.op)
	{
	case CompareOp::Greater:      return a > b;
	case CompareOp::Less:         return a < b;
	case CompareOp::Equal:        return std::abs(a - b) < epsilon;
	case CompareOp::NotEqual:     return std::abs(a - b) >= epsilon;
	case CompareOp::GreaterEqual: return a >= b;
	case CompareOp::LessEqual:    return a <= b;
	}
	return false;
}
// ノードIDから入力値をfloatとして解決する
float ConditionGraphEvaluator::ResolveInputValue(const ConditionGraph& graph, int nodeId, entt::registry& registry, entt::entity entity)
{
	for (const auto& g : graph.getterNodes)
		if (g.id == nodeId) return GetGetterValue(g, registry, entity);

	for (const auto& v : graph.variableNodes)
		if (v.id == nodeId) return GetVariableValue(v);

	for (const auto& o : graph.operatorNodes)
		if (o.id == nodeId) return EvaluateOperator(graph, o, registry, entity) ? 1.0f : 0.0f;

	return 0.0f; // どのノードにも見つからなかった場合
}
