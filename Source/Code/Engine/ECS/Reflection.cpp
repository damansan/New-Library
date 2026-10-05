#include "Reflection.h"
#include <DirectXMath.h>

// 値の型を判別するヘルパー
namespace
{
	ValueType ResolveValueType(entt::meta_type type)
	{
		if (type == entt::resolve<bool>())              return ValueType::Bool;
		if (type == entt::resolve<int>())               return ValueType::Int;
		if (type == entt::resolve<DirectX::XMFLOAT2>()) return ValueType::Float2;
		if (type == entt::resolve<DirectX::XMFLOAT3>()) return ValueType::Float3;
		if (type == entt::resolve<DirectX::XMFLOAT4>()) return ValueType::Float4;
		return ValueType::Float;
	}

	// leaf型かどうか
	bool IsLeafType(entt::meta_type type)
	{
		return type == entt::resolve<bool>()
			|| type == entt::resolve<int>()
			|| type == entt::resolve<float>()
			|| type == entt::resolve<std::string>()
			|| type == entt::resolve<DirectX::XMFLOAT2>()
			|| type == entt::resolve<DirectX::XMFLOAT3>()
			|| type == entt::resolve<DirectX::XMFLOAT4>();
	}
}

// 登録済みのフィールド一覧を取得する
const std::vector<FieldEntry>& Reflection::GetAllFields()
{
	static std::vector<FieldEntry> fieldEntries;

	if (fieldEntries.empty())
	{
		for (auto&& [id, metaType] : entt::resolve())
		{
			TypeInfo* typeInfo = metaType.custom();

			if (!typeInfo) continue;

			std::string category = typeInfo->category;
			std::string componentName = std::string(metaType.name());

			if (componentName.empty()) continue;

			for (auto&& [fieldId, metaData] : metaType.data())
			{
				FieldInfo* fieldInfo = metaData.custom();
				std::string label = fieldInfo ? fieldInfo->label : std::string(metaData.name());

				entt::meta_type fieldType = metaData.type();

				if (!IsLeafType(fieldType) && fieldType && !fieldType.is_enum() && fieldType.data().begin() != fieldType.data().end())
				{
					// ネストしたフィールド：内側の型が持つフィールドを、1段階だけ掘り下げて追加する
					for (auto&& [subFieldId, subMetaData] : fieldType.data())
					{
						FieldInfo* subFieldInfo = subMetaData.custom();
						std::string subLabel = subFieldInfo ? subFieldInfo->label : std::string(subMetaData.name());

						ValueType valueType = ResolveValueType(subMetaData.type());
						std::string combinedLabel = label + "." + subLabel;

						fieldEntries.push_back(FieldEntry{
							metaType, fieldId, fieldType, subFieldId, componentName, category, combinedLabel, valueType
							});
					}
					continue;
				}

				ValueType valueType = ResolveValueType(fieldType);

				fieldEntries.push_back(FieldEntry{
					metaType, fieldId, entt::meta_type{}, 0, componentName, category, label, valueType
					});
			}
		}
	}

	return fieldEntries;
}

// Float2/3/4型の、成分数を返す(それ以外は0)
int Reflection::GetComponentCount(ValueType type)
{
	switch (type)
	{
	case ValueType::Float2: return 2;
	case ValueType::Float3: return 3;
	case ValueType::Float4: return 4;
	default: return 0;
	}
}