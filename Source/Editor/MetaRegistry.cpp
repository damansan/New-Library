#include "MetaRegistry.h"
#include <DirectXMath.h>
#include <filesystem>
#include <imgui.h>

using namespace entt::literals;

#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/MeshComponent.h"
#include "ECS/Components/MovementComponent.h"
#include "ECS/Components/InputComponent.h"
#include "ECS/Components/AnimatorComponent.h"

void MetaRegistry::Initialize()
{
	entt::meta_factory<TransformComponent>{}
	.type("TransformComponent")
		.custom<TypeInfo>(TypeInfo{ "Transform" })
		.data<&TransformComponent::position>("position"_hs)
		.custom<FieldInfo>(FieldInfo{ "Position", 0.0f, 0.0f, 0.1f })
		.data<&TransformComponent::rotation>("rotation"_hs)
		.custom<FieldInfo>(FieldInfo{ "Rotation", 0.0f, 0.0f, 0.01f })
		.data<&TransformComponent::scale>("scale"_hs)
		.custom<FieldInfo>(FieldInfo{ "Scale", 0.0f, 0.0f, 0.01f });

	entt::meta_factory<MeshComponent>{}
	.type("MeshComponent")
		.custom<TypeInfo>(TypeInfo{ "Mesh" })
		.data<&MeshComponent::modelPath>("modelPath"_hs)
		.custom<FieldInfo>(FieldInfo{ "Model", 0.0f, 0.0f, 0.1f, "MODEL_PATH" });

	entt::meta_factory<MovementComponent>{}
	.type("MovementComponent")
		.custom<TypeInfo>(TypeInfo{ "Movement" })
		.data<&MovementComponent::moveSpeed>("moveSpeed"_hs)
		.custom<FieldInfo>(FieldInfo{ "Move Speed", 0.0f, 20.0f })
		.data<&MovementComponent::jumpPower>("jumpPower"_hs)
		.custom<FieldInfo>(FieldInfo{ "Jump Power", 0.0f, 50.0f })
		.data<&MovementComponent::gravity>("gravity"_hs)
		.custom<FieldInfo>(FieldInfo{ "Gravity", -100.0f, 0.0f })
		.data<&MovementComponent::velocity>("velocity"_hs)
		.custom<FieldInfo>(FieldInfo{ "Velocity", 0.0f, 0.0f, 0.01f })
		.data<&MovementComponent::isGrounded>("isGrounded"_hs)
		.custom<FieldInfo>(FieldInfo{ "Is Grounded" });

	entt::meta_factory<InputComponent>{}
	.type("InputComponent")
		.custom<TypeInfo>(TypeInfo{ "Input" })
		.data<&InputComponent::moveX>("moveX"_hs)
		.custom<FieldInfo>(FieldInfo{ "MoveX", 0.0f, 0.0f, 0.1f, "", true })
		.data<&InputComponent::moveZ>("moveZ"_hs)
		.custom<FieldInfo>(FieldInfo{ "MoveZ", 0.0f, 0.0f, 0.1f, "", true })
		.data<&InputComponent::jump>("jump"_hs)
		.custom<FieldInfo>(FieldInfo{ "Jump", 0.0f, 0.0f, 0.1f, "", true })
		.data<&InputComponent::attack>("attack"_hs)
		.custom<FieldInfo>(FieldInfo{ "Attack", 0.0f, 0.0f, 0.1f, "", true });

	entt::meta_factory<AnimatorComponent>{}
	.type("AnimatorComponent")
		.custom<TypeInfo>(TypeInfo{ "Animator" })
		.data<&AnimatorComponent::playSpeed>("playSpeed"_hs)
		.custom<FieldInfo>(FieldInfo{ "Play Speed", 0.0f, 3.0f })
		.data<&AnimatorComponent::loop>("loop"_hs)
		.custom<FieldInfo>(FieldInfo{ "Loop" })
		.data<&AnimatorComponent::useRootMotion>("useRootMotion"_hs)
		.custom<FieldInfo>(FieldInfo{ "Use Root Motion" })
		.data<&AnimatorComponent::useRootMotionY>("useRootMotionY"_hs)
		.custom<FieldInfo>(FieldInfo{ "Use Root Motion Y" })
		.data<&AnimatorComponent::rootMotionNodeName>("rootMotionNodeName"_hs)
		.custom<FieldInfo>(FieldInfo{ "Root Motion Node" });
}

// ìoò^çœÇ›ÉÅÉ^èÓïÒÇ©ÇÁInspectorUIÇé©ìÆï`âÊ
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
		else
		{
			ImGui::TextDisabled("%s: (unsupported type)", label.c_str());
		}
	}
}

// ìoò^çœÇ›ÇÃÉtÉBÅ[ÉhàÍóóÇéÊìæÇ∑ÇÈ
const std::vector<FieldEntry>& MetaRegistry::GetAllFields()
{
	static std::vector<FieldEntry> fieldEntries;

	if (fieldEntries.empty())
	{
		for (auto&& [id, metaType] : entt::resolve())
		{
			TypeInfo* typeInfo = metaType.custom();
			std::string category = typeInfo ? typeInfo->category : "";
			std::string componentName = std::string(metaType.name());

			if (componentName.empty()) continue;

			for (auto&& [fieldId, metaData] : metaType.data())
			{
				FieldInfo* fieldInfo = metaData.custom();
				std::string label = fieldInfo ? fieldInfo->label : std::string(metaData.name());

				entt::meta_type fieldType = metaData.type();
				ValueType valueType = ValueType::Float;
				if (fieldType == entt::resolve<bool>())                   valueType = ValueType::Bool;
				else if (fieldType == entt::resolve<int>())               valueType = ValueType::Int;
				else if (fieldType == entt::resolve<DirectX::XMFLOAT2>()) valueType = ValueType::Float2;
				else if (fieldType == entt::resolve<DirectX::XMFLOAT3>()) valueType = ValueType::Float3;
				else if (fieldType == entt::resolve<DirectX::XMFLOAT4>()) valueType = ValueType::Float4;

				fieldEntries.push_back(FieldEntry{
					metaType, fieldId, componentName, category, label, valueType
					});
			}
		}
	}

	return fieldEntries;
}

// Float2/3/4å^ÇÃÅAê¨ï™êîÇï‘Ç∑(ÇªÇÍà»äOÇÕ0)
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
