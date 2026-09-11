#include "MetaRegistry.h"
#include <DirectXMath.h>
#include <filesystem>
#include <imgui.h>

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

void MetaRegistry::Initialize()
{
	// トランスフォームコンポーネントを登録
	RegisterTransformComponent();
	// メッシュコンポーネントを登録
	RegisterMeshComponent();
	// 移動コンポーネントを登録
	RegisterMovementComponent();
	// プレイヤー待機コンポーネントを登録
	RegisterPlayerIdleComponent();
	// プレイヤー移動コンポーネントを登録
	RegisterPlayerMoveComponent();
	// プレイヤー回避コンポーネントを登録
	RegisterPlayerDodgeComponent();
	// プレイヤー全力疾走コンポーネントを登録
	RegisterPlayerSprintComponent();
	// ゲーム入力コンポーネントを登録
	RegisterGameInputComponent();
	// アニメーターコンポーネントを登録
	RegisterAnimatorComponent();
	// シーケンサーコンポーネントを登録
	RegisterSequencerComponent();
	// アニメーション速度コンポーネントを登録
	RegisterAnimationSpeedComponent();
}

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

				if (!IsLeafType(fieldType) && fieldType && fieldType.data().begin() != fieldType.data().end())
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

// トランスフォームコンポーネントを登録
void MetaRegistry::RegisterTransformComponent()
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
}
// メッシュコンポーネントを登録
void MetaRegistry::RegisterMeshComponent()
{
	entt::meta_factory<MeshComponent>{}
	.type("MeshComponent")
		.custom<TypeInfo>(TypeInfo{ "Mesh" })
		.data<&MeshComponent::modelPath>("modelPath"_hs)
		.custom<FieldInfo>(FieldInfo{ "Model", 0.0f, 0.0f, 0.1f, "MODEL_PATH" });
}
// 移動コンポーネントを登録
void MetaRegistry::RegisterMovementComponent()
{
	entt::meta_factory<MovementComponent>{}
	.type("MovementComponent")
		.custom<TypeInfo>(TypeInfo{ "Movement" })
		.data<&MovementComponent::jumpPower>("jumpPower"_hs)
		.custom<FieldInfo>(FieldInfo{ "Jump Power", 0.0f, 50.0f })
		.data<&MovementComponent::gravity>("gravity"_hs)
		.custom<FieldInfo>(FieldInfo{ "Gravity", -100.0f, 0.0f })
		.data<&MovementComponent::velocity>("velocity"_hs)
		.custom<FieldInfo>(FieldInfo{ "Velocity", 0.0f, 0.0f, 0.01f })
		.data<&MovementComponent::isGrounded>("isGrounded"_hs)
		.custom<FieldInfo>(FieldInfo{ "Is Grounded" });
}

// プレイヤー待機コンポーネントを登録
void MetaRegistry::RegisterPlayerIdleComponent()
{
	entt::meta_factory<PlayerIdleComponent>{}
	.type("PlayerIdleComponent")
		.custom<TypeInfo>(TypeInfo{ "Player Idle" })
		.data<&PlayerIdleComponent::isIdling>("isIdling"_hs)
		.custom<FieldInfo>(FieldInfo{ "Is Idling", 0.0f, 0.0f, 0.1f, "", true });
}
// プレイヤー移動コンポーネントを登録
void MetaRegistry::RegisterPlayerMoveComponent()
{
	entt::meta_factory<PlayerMoveComponent>{}
	.type("PlayerMoveComponent")
		.custom<TypeInfo>(TypeInfo{ "Player Move" })
		.data<&PlayerMoveComponent::walkSpeed>("walkSpeed"_hs)
		.custom<FieldInfo>(FieldInfo{ "Walk Speed", 0.0f, 10.0f })
		.data<&PlayerMoveComponent::jogSpeed>("jogSpeed"_hs)
		.custom<FieldInfo>(FieldInfo{ "Jog Speed", 0.0f, 10.0f })
		.data<&PlayerMoveComponent::jogThreshold>("jogThreshold"_hs)
		.custom<FieldInfo>(FieldInfo{ "Jog Threshold", 0.0f, 1.0f })
		.data<&PlayerMoveComponent::turnSpeed>("turnSpeed"_hs)
		.custom<FieldInfo>(FieldInfo{ "Turn Speed", 0.0f, 30.0f })
		.data<&PlayerMoveComponent::inputLossGraceTime>("inputLossGraceTime"_hs)
		.custom<FieldInfo>(FieldInfo{ "Input Loss Grace Time", 0.0f, 1.0f })
		.data<&PlayerMoveComponent::isWalking>("isWalking"_hs)
		.custom<FieldInfo>(FieldInfo{ "Is Walking" })
		.data<&PlayerMoveComponent::isJogging>("isJogging"_hs)
		.custom<FieldInfo>(FieldInfo{ "Is Jogging" })
		.data<&PlayerMoveComponent::isMoving>("isMoving"_hs)
		.custom<FieldInfo>(FieldInfo{ "Is Moving" });
}
// プレイヤー回避コンポーネントを登録
void MetaRegistry::RegisterPlayerDodgeComponent()
{
	entt::meta_factory<PlayerDodgeComponent>{}
	.type("PlayerDodgeComponent")
		.custom<TypeInfo>(TypeInfo{ "Player Dodge" })
		.data<&PlayerDodgeComponent::directionBufferTime>("directionBufferTime"_hs)
		.custom<FieldInfo>(FieldInfo{ "Direction Buffer Time", 0.0f, 1.0f })
		.data<&PlayerDodgeComponent::isDodging>("isDodging"_hs)
		.custom<FieldInfo>(FieldInfo{ "Is Dodging", 0.0f, 0.0f, 0.1f, "", true })
		.data<&PlayerDodgeComponent::isNoneInput>("isNoneInput"_hs)
		.custom<FieldInfo>(FieldInfo{ "Is None Input", 0.0f, 0.0f, 0.1f, "", true })
		.data<&PlayerDodgeComponent::isFrontInput>("isFrontInput"_hs)
		.custom<FieldInfo>(FieldInfo{ "Is Front Input", 0.0f, 0.0f, 0.1f, "", true })
		.data<&PlayerDodgeComponent::isBackInput>("isBackInput"_hs)
		.custom<FieldInfo>(FieldInfo{ "Is Back Input", 0.0f, 0.0f, 0.1f, "", true })
		.data<&PlayerDodgeComponent::isLeftInput>("isLeftInput"_hs)
		.custom<FieldInfo>(FieldInfo{ "Is Left Input", 0.0f, 0.0f, 0.1f, "", true })
		.data<&PlayerDodgeComponent::isRightInput>("isRightInput"_hs)
		.custom<FieldInfo>(FieldInfo{ "Is Right Input", 0.0f, 0.0f, 0.1f, "", true });
}
// プレイヤー全力疾走コンポーネントを登録
void MetaRegistry::RegisterPlayerSprintComponent()
{
	entt::meta_factory<PlayerSprintComponent>{}
	.type("PlayerSprintComponent")
		.custom<TypeInfo>(TypeInfo{ "Player Sprint" })
		.data<&PlayerSprintComponent::sprintSpeed>("sprintSpeed"_hs)
		.custom<FieldInfo>(FieldInfo{ "Sprint Speed", 0.0f, 20.0f })
		.data<&PlayerSprintComponent::turnSpeed>("turnSpeed"_hs)
		.custom<FieldInfo>(FieldInfo{ "Turn Speed", 0.0f, 30.0f })
		.data<&PlayerSprintComponent::isSprinting>("isSprinting"_hs)
		.custom<FieldInfo>(FieldInfo{ "Is Sprinting", 0.0f, 0.0f, 0.1f, "", true });
}
// ゲーム入力コンポーネントを登録
void MetaRegistry::RegisterGameInputComponent()
{
	entt::meta_factory<StickState>{}
	.type("StickState")
		.data<&StickState::x>("x"_hs)
		.custom<FieldInfo>(FieldInfo{ "X", 0.0f, 0.0f, 0.1f, "", true })
		.data<&StickState::y>("y"_hs)
		.custom<FieldInfo>(FieldInfo{ "Y", 0.0f, 0.0f, 0.1f, "", true })
		.data<&StickState::hasInput>("hasInput"_hs)
		.custom<FieldInfo>(FieldInfo{ "Has Input", 0.0f, 0.0f, 0.1f, "", true });

	entt::meta_factory<ButtonState>{}
	.type("ButtonState")
		.data<&ButtonState::pressed>("pressed"_hs)
		.custom<FieldInfo>(FieldInfo{ "Pressed", 0.0f, 0.0f, 0.1f, "", true })
		.data<&ButtonState::held>("held"_hs)
		.custom<FieldInfo>(FieldInfo{ "Held", 0.0f, 0.0f, 0.1f, "", true })
		.data<&ButtonState::released>("released"_hs)
		.custom<FieldInfo>(FieldInfo{ "Released", 0.0f, 0.0f, 0.1f, "", true });

	entt::meta_factory<TriggerState>{}
	.type("TriggerState")
		.data<&TriggerState::value>("value"_hs)
		.custom<FieldInfo>(FieldInfo{ "Value", 0.0f, 0.0f, 0.1f, "", true })
		.data<&TriggerState::pressed>("pressed"_hs)
		.custom<FieldInfo>(FieldInfo{ "Pressed", 0.0f, 0.0f, 0.1f, "", true })
		.data<&TriggerState::held>("held"_hs)
		.custom<FieldInfo>(FieldInfo{ "Held", 0.0f, 0.0f, 0.1f, "", true })
		.data<&TriggerState::released>("released"_hs)
		.custom<FieldInfo>(FieldInfo{ "Released", 0.0f, 0.0f, 0.1f, "", true });

	entt::meta_factory<GameInputComponent>{}
	.type("GameInputComponent")
		.custom<TypeInfo>(TypeInfo{ "Input" })
		.data<&GameInputComponent::moveStick>("moveStick"_hs)
		.custom<FieldInfo>(FieldInfo{ "Move Stick" })
		.data<&GameInputComponent::cameraStick>("cameraStick"_hs)
		.custom<FieldInfo>(FieldInfo{ "Camera Stick" })
		.data<&GameInputComponent::dodgeTrigger>("dodgeTrigger"_hs)
		.custom<FieldInfo>(FieldInfo{ "Dodge Trigger" })
		.data<&GameInputComponent::jump>("jump"_hs)
		.custom<FieldInfo>(FieldInfo{ "Jump", 0.0f, 0.0f, 0.1f, "", true })
		.data<&GameInputComponent::attack>("attack"_hs)
		.custom<FieldInfo>(FieldInfo{ "Attack", 0.0f, 0.0f, 0.1f, "", true });
}
// アニメーターコンポーネントを登録
void MetaRegistry::RegisterAnimatorComponent()
{
	entt::meta_factory<AnimatorComponent>{}
	.type("AnimatorComponent")
		.custom<TypeInfo>(TypeInfo{ "Animator" })
		.data<&AnimatorComponent::animatorGraphPath>("animatorGraphPath"_hs)
		.custom<FieldInfo>(FieldInfo{ "Animator Graph", 0.0f, 0.0f, 0.1f, "ANIMGRAPH_PATH" })
		.data<&AnimatorComponent::playSpeed>("playSpeed"_hs)
		.custom<FieldInfo>(FieldInfo{ "Play Speed", 0.0f, 3.0f })
		.data<&AnimatorComponent::loop>("loop"_hs)
		.custom<FieldInfo>(FieldInfo{ "Loop" })
		.data<&AnimatorComponent::useRootMotion>("useRootMotion"_hs)
		.custom<FieldInfo>(FieldInfo{ "Use Root Motion" })
		.data<&AnimatorComponent::useRootMotionY>("useRootMotionY"_hs)
		.custom<FieldInfo>(FieldInfo{ "Use Root Motion Y" })
		.data<&AnimatorComponent::rootMotionNodeName>("rootMotionNodeName"_hs)
		.custom<FieldInfo>(FieldInfo{ "Root Motion Node" })
		.data<&AnimatorComponent::isBlending>("isBlending"_hs)
		.custom<FieldInfo>(FieldInfo{ "Is Blending", 0.0f, 0.0f, 0.1f, "", true })
		.data<&AnimatorComponent::blendRate>("blendRate"_hs)
		.custom<FieldInfo>(FieldInfo{ "Blend Rate", 0.0f, 1.0f, 0.1f, "", true })
		.data<&AnimatorComponent::isEnd>("isEnd"_hs)
		.custom<FieldInfo>(FieldInfo{ "Is End", 0.0f, 0.0f, 0.1f, "", true })
		.data<&AnimatorComponent::blendAnimation>("blendAnimation"_hs)
		.custom<FieldInfo>(FieldInfo{ "Blend From (Old Clip)", 0.0f, 0.0f, 0.1f, "", true });
}
// シーケンサーコンポーネントを登録
void MetaRegistry::RegisterSequencerComponent()
{
	entt::meta_factory<SequencerComponent>{}
	.type("SequencerComponent")
		.custom<TypeInfo>(TypeInfo{ "Sequencer" })
		.data<&SequencerComponent::sequencerPath>("sequencerPath"_hs)
		.custom<FieldInfo>(FieldInfo{ "Sequencer File", 0.0f, 0.0f, 0.1f, "SEQUENCER_PATH" })
		.data<&SequencerComponent::animExit>("animExit"_hs)
		.custom<FieldInfo>(FieldInfo{ "Anim Exit", 0.0f, 0.0f, 0.1f, "", true })
		.data<&SequencerComponent::attack>("attack"_hs)
		.custom<FieldInfo>(FieldInfo{ "Attack", 0.0f, 0.0f, 0.1f, "", true })
		.data<&SequencerComponent::superArmor>("superArmor"_hs)
		.custom<FieldInfo>(FieldInfo{ "SuperArmor", 0.0f, 0.0f, 0.1f, "", true });
}
// アニメーション速度コンポーネントを登録
void MetaRegistry::RegisterAnimationSpeedComponent()
{
	entt::meta_factory<AnimationSpeedComponent>{}
	.type("AnimationSpeedComponent")
		.custom<TypeInfo>(TypeInfo{ "Animation Speed" })
		.data<&AnimationSpeedComponent::animationSpeedPath>("animationSpeedPath"_hs)
		.custom<FieldInfo>(FieldInfo{ "Speed Curve File", 0.0f, 0.0f, 0.1f, "ANIMSPEED_PATH" });
}