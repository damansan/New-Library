#include "MetaRegistry.h"
#include <DirectXMath.h>
#include <filesystem>
#include <imgui.h>
#include <vector>
#include <utility>

using namespace entt::literals;

#include "Engine/ECS/Components/TransformComponent.h"
#include "Engine/ECS/Components/MeshComponent.h"
#include "Engine/ECS/Components/MovementComponent.h"

#include "Project/ECS/Components/PlayerIdleComponent.h"
#include "Project/ECS/Components/PlayerMoveComponent.h"
#include "Project/ECS/Components/PlayerDodgeComponent.h"
#include "Project/ECS/Components/PlayerSprintComponent.h"

#include "Project/ECS/Components/GameInputComponent.h"
#include "Engine/ECS/Components/AnimatorComponent.h"
#include "Engine/ECS/Components/SequencerComponent.h"
#include "Engine/ECS/Components/AnimationSpeedComponent.h"

// 登録済みメタ情報からInspectorUIを自動描画
void MetaRegistry::DrawMetaFields(entt::meta_type metaType, entt::meta_any instance)
{
	if (!metaType) return;

	for (auto&& [id, metaData] : metaType.data())
	{
		FieldInfo* fieldInfo = metaData.custom();
		std::string label = fieldInfo ? fieldInfo->label : std::string(metaData.name());

		entt::meta_any value = metaData.get(instance);

		if (float* f = value.try_cast<float>())
		{
			if (fieldInfo && fieldInfo->readOnly)
			{
				ImGui::Text("%s: %.2f", label.c_str(), *f);
			}
			else
			{
				float minV = fieldInfo ? fieldInfo->minValue : 0.0f;
				float maxV = fieldInfo ? fieldInfo->maxValue : 0.0f;
				float speed = fieldInfo ? fieldInfo->dragSpeed : 0.1f;

				if (ImGui::DragFloat(label.c_str(), f, speed, minV, maxV))
				{
					metaData.set(instance, *f);
				}
			}
		}
		else if (DirectX::XMFLOAT3* v = value.try_cast<DirectX::XMFLOAT3>())
		{
			float speed = fieldInfo ? fieldInfo->dragSpeed : 0.1f;

			if (ImGui::DragFloat3(label.c_str(), &v->x, speed))
			{
				metaData.set(instance, *v);
			}
		}
		else if (bool* b = value.try_cast<bool>())
		{
			if (fieldInfo && fieldInfo->readOnly)
			{
				ImGui::Text("%s: %s", label.c_str(), *b ? "true" : "false");
			}
			else
			{
				if (ImGui::Checkbox(label.c_str(), b))
				{
					metaData.set(instance, *b);
				}
			}
		}
		else if (std::string* s = value.try_cast<std::string>())
		{
			if (fieldInfo && !fieldInfo->dragDropPayload.empty())
			{
				std::string displayName = s->empty()
					? "None"
					: std::filesystem::path(*s).stem().string();

				ImGui::InputText(label.c_str(), displayName.data(),
					displayName.size() + 1, ImGuiInputTextFlags_ReadOnly);

				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(fieldInfo->dragDropPayload.c_str()))
					{
						std::string newPath(static_cast<const char*>(payload->Data));
						metaData.set(instance, newPath);
					}
					ImGui::EndDragDropTarget();
				}
			}
			else if (fieldInfo && fieldInfo->readOnly)
			{
				ImGui::Text("%s: %s", label.c_str(), s->c_str());
			}
			else
			{
				char buf[256];
				strncpy_s(buf, s->c_str(), sizeof(buf) - 1);
				if (ImGui::InputText(label.c_str(), buf, sizeof(buf)))
				{
					metaData.set(instance, std::string(buf));
				}
			}
		}
		else if (value.type().is_enum())
		{
			entt::meta_type enumType = value.type();

			// enumの各値(REFLECT_ENUM_VALUEで登録した定数)を、ラベル付きで列挙する
			std::vector<std::pair<std::string, entt::meta_any>> options;
			std::string currentLabel = "(Unknown)";

			for (auto&& [enumId, enumData] : enumType.data())
			{
				EnumValueInfo* enumInfo = enumData.custom();
				std::string optLabel = enumInfo ? enumInfo->label : std::string(enumData.name());

				entt::meta_any optValue = enumData.get({});
				options.push_back({ optLabel, optValue });

				if (optValue == value)
				{
					currentLabel = optLabel;
				}
			}

			if (ImGui::BeginCombo(label.c_str(), currentLabel.c_str()))
			{
				for (auto& [optLabel, optValue] : options)
				{
					bool selected = (optLabel == currentLabel);
					if (ImGui::Selectable(optLabel.c_str(), selected))
					{
						metaData.set(instance, optValue);
					}
					if (selected) ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
		}
		else
		{
			ImGui::TextDisabled("%s: (unsupported type)", label.c_str());
		}
	}
}

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

// 登録済みのフィード一覧を取得する
const std::vector<FieldEntry>& MetaRegistry::GetAllFields()
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
int MetaRegistry::GetComponentCount(ValueType type)
{
	switch (type)
	{
	case ValueType::Float2: return 2;
	case ValueType::Float3: return 3;
	case ValueType::Float4: return 4;
	default: return 0;
	}
}