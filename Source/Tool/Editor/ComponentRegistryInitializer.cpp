#include "ComponentRegistryInitializer.h"
#include "Engine/Resource/ComponentRegistry.h"
#include "Tool/Editor/ComponentInspectorRegistry.h"

#include <imgui.h>
#include <Windows.h>
#include <commdlg.h>
#include <filesystem>
#include <algorithm>

#include "Engine/ECS/Components/CategoryComponent.h"
#include "Engine/ECS/Components/TransformComponent.h"
#include "Engine/ECS/Components/MeshComponent.h"
#include "Engine/ECS/Components/MaterialComponent.h"

#include "Project/ECS/Components/GameInputComponent.h"
#include "Engine/ECS/Components/ControllerComponent.h"
#include "Engine/ECS/Components/MovementComponent.h"

#include "Project/ECS/Components/PlayerIdleComponent.h"
#include "Project/ECS/Components/PlayerMoveComponent.h"
#include "Project/ECS/Components/PlayerDodgeComponent.h"
#include "Project/ECS/Components/PlayerSprintComponent.h"

#include "Engine/ECS/Components/AnimatorComponent.h"
#include "Engine/ECS/Components/SkinnedMeshComponent.h"

#include "Engine/ECS/Components/FreeCameraComponent.h"
#include "Project/ECS/Components/GameCameraComponent.h"

#include "Engine/Core/PhysicsDestroyQueue.h"
#include "Engine/ECS/Components/PhysicsComponents.h"

#include "Engine/ECS/Components/IKComponent.h"

#include "Engine/ECS/Components/SequencerComponent.h"
#include "Engine/ECS/Components/AnimationSpeedComponent.h"

#include "MetaRegistry.h"

#define REGISTER_COMPONENT(Type, Name, DrawFunc)                                    \
    ComponentRegistry::Register(Name,                                               \
    {                                                                               \
        Name,                                                                       \
        [](auto& r, auto e){ if (!r.any_of<Type>(e)) r.emplace<Type>(e); },         \
        [](auto& r, auto e){ r.remove<Type>(e); },                                  \
        [](auto& r, auto e){ return r.any_of<Type>(e); },                           \
        [](entt::registry& r, entt::entity e) -> entt::meta_any                     \
        {                                                                           \
            auto* c = r.try_get<Type>(e);                                           \
            if (!c) return entt::meta_any{};                                        \
            return entt::forward_as_meta(*c);                                       \
        }                                                                           \
    });                                                                             \
    ComponentInspectorRegistry::Register(Name, DrawFunc);                           \
    ComponentRegistry::RegisterTypeName(#Type, Name)

// 独自UIが不要な、シンプルなコンポーネント用
#define REGISTER_COMPONENT_AUTO(Type, Name)                                         \
    ComponentRegistry::Register(Name,                                               \
    {                                                                               \
        Name,                                                                       \
        [](auto& r, auto e){ if (!r.any_of<Type>(e)) r.emplace<Type>(e); },         \
        [](auto& r, auto e){ r.remove<Type>(e); },                                  \
        [](auto& r, auto e){ return r.any_of<Type>(e); },                           \
        [](entt::registry& r, entt::entity e) -> entt::meta_any                     \
        {                                                                           \
            auto* c = r.try_get<Type>(e);                                           \
            if (!c) return entt::meta_any{};                                        \
            return entt::forward_as_meta(*c);                                       \
        }                                                                           \
    });                                                                             \
    ComponentRegistry::RegisterTypeName(#Type, Name)

#define DRAW_COMPONENT(Type, Label, body)                                           \


#define DRAW_COMPONENT(Type, Label, body)                                           \
    [](auto& reg, auto e)                                                           \
    {                                                                               \
        auto* _c = reg.try_get<Type>(e);                                            \
        if (!_c) return;                                                            \
        if (ImGui::CollapsingHeader(Label, ImGuiTreeNodeFlags_DefaultOpen))         \
        { body }                                                                    \
    }

void ComponentRegistryInitializer::Initialize()
{
	// カテゴリコンポーネントの登録
	ComponentRegistry::Register("Category",
		{
			"Category",
			[](auto& reg, auto e) { if (!reg.any_of<ComponentCategory>(e)) reg.emplace<ComponentCategory>(e); },
			[](auto& reg, auto e) { reg.remove<ComponentCategory>(e); },
			[](auto& reg, auto e) { return reg.any_of<ComponentCategory>(e); },
			[](entt::registry& r, entt::entity e) -> entt::meta_any
			{
				auto* c = r.try_get<ComponentCategory>(e);
				if (!c) return entt::meta_any{};
				return entt::forward_as_meta(*c);
			}
		});
	ComponentInspectorRegistry::Register("Category",
		[](auto& reg, auto e)
		{
			auto* _c = reg.try_get<ComponentCategory>(e);
			if (!_c) return;

			const char* categoryNames[] = { "None", "Player", "Enemy", "Stage", "Object", "Camera" };
			std::string header = "Category: " + std::string(categoryNames[(int)_c->category]);

			if (!ImGui::CollapsingHeader(header.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) return;

			int category = (int)_c->category;
			if (ImGui::Combo("Category", &category, categoryNames, 6))
				_c->category = (EntityCategory)category;
		});

	// トランスフォームコンポーネントの登録
	REGISTER_COMPONENT_AUTO(TransformComponent, "Transform");

	// メッシュコンポーネントの登録
	REGISTER_COMPONENT_AUTO(MeshComponent, "Mesh");

	// マテリアルコンポーネントの登録
	ComponentRegistry::Register("Material",
		{
			"Material",
			[](auto& reg, auto e) { if (!reg.any_of<MaterialComponent>(e)) reg.emplace<MaterialComponent>(e); },
			[](auto& reg, auto e) { reg.remove<MaterialComponent>(e); },
			[](auto& reg, auto e) { return reg.any_of<MaterialComponent>(e); },
			[](entt::registry& r, entt::entity e) -> entt::meta_any
			{
				auto* c = r.try_get<MaterialComponent>(e);
				if (!c) return entt::meta_any{};
				return entt::forward_as_meta(*c);
			}
		});
	ComponentInspectorRegistry::Register("Material",
		[](auto& reg, auto e)
		{
			auto* mat = reg.try_get<MaterialComponent>(e);
			if (!mat) return;
			if (!ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen)) return;

			const char* shaderNames[] = { "Default (PBR)", "Basic" };
			int current = (int)mat->shadingModel;
			if (ImGui::Combo("Shader", &current, shaderNames, 2))
			{
				mat->shadingModel = (ShadingModel)current;
			}
		});

	// Player待機コンポーネントの登録
	REGISTER_COMPONENT_AUTO(PlayerIdleComponent, "Player Idle");
	// Player移動コンポーネントの登録
	REGISTER_COMPONENT_AUTO(PlayerMoveComponent, "Player Move");
	// Player回避コンポーネントの登録
	REGISTER_COMPONENT_AUTO(PlayerDodgeComponent, "Player Dodge");
	// Sprintコンポーネントの登録
	REGISTER_COMPONENT_AUTO(PlayerSprintComponent, "Player Sprint");

	// インプットコンポーネントの登録
	REGISTER_COMPONENT_AUTO(GameInputComponent, "GameInput");

	// アニメーターコンポーネントの登録
	REGISTER_COMPONENT(AnimatorComponent, "Animator",
		[](auto& reg, auto e)
		{
			auto* anim = reg.try_get<AnimatorComponent>(e);
			auto* skinned = reg.try_get<SkinnedMeshComponent>(e);
			if (!anim) return;

			if (!ImGui::CollapsingHeader("Animator", ImGuiTreeNodeFlags_DefaultOpen)) return;

			if (!skinned || !skinned->model)
			{
				ImGui::TextColored({ 1, 0.5f, 0, 1 }, "No SkinnedMesh found.");
				return;
			}
			const auto& animations = skinned->model->GetAnimations();
			if (animations.empty())
			{
				ImGui::TextColored({ 1, 0.5f, 0, 1 }, "No animations found.");
				return;
			}

			if (ImGui::BeginCombo("Animation", anim->currentAnimation.c_str()))
			{
				for (const auto& animation : animations)
				{
					bool selected = (anim->currentAnimation == animation.name);
					if (ImGui::Selectable(animation.name.c_str(), selected))
					{
						anim->currentAnimation = animation.name;
						anim->animationTime = 0.0f;
						anim->isPlaying = true;
						anim->isEnd = false;
					}
					if (selected) ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}

			if (anim->isPlaying)
			{
				if (ImGui::Button("Stop")) anim->isPlaying = false;
			}
			else
			{
				if (ImGui::Button("Play")) { anim->isPlaying = true; anim->isEnd = false; }
			}
			MetaRegistry::DrawMetaFields(entt::resolve<AnimatorComponent>(), entt::forward_as_meta(*anim));
		});

	// フリーカメラコンポーネントの登録
	REGISTER_COMPONENT(FreeCameraComponent, "FreeCamera",
		DRAW_COMPONENT(FreeCameraComponent, "FreeCamera",
			{
				ImGui::DragFloat("Camera Speed", &_c->cameraSpeed, 0.1f, 0.1f, 100.0f);
				ImGui::DragFloat("Yaw",          &_c->yaw,         0.01f);
				ImGui::DragFloat("Pitch",        &_c->pitch,       0.01f, -1.5f, 1.5f);
				ImGui::Separator();
				ImGui::DragFloat3("Eye",    &_c->base.currentEye.x,    0.1f);
				ImGui::DragFloat3("Target", &_c->base.currentTarget.x, 0.1f);
			}));

	// ゲームカメラコンポーネントの登録
	ComponentRegistry::Register("GameCamera",
		{
			"GameCamera",
			[](auto& reg, auto e) { if (!reg.any_of<GameCameraComponent>(e)) reg.emplace<GameCameraComponent>(e); },
			[](auto& reg, auto e) { reg.remove<GameCameraComponent>(e); },
			[](auto& reg, auto e) { return reg.any_of<GameCameraComponent>(e); },
			[](entt::registry& r, entt::entity e) -> entt::meta_any
			{
				auto* c = r.try_get<GameCameraComponent>(e);
				if (!c) return entt::meta_any{};
				return entt::forward_as_meta(*c);
			}
		});
	ComponentInspectorRegistry::Register("GameCamera",
		[](auto& reg, auto e)
		{
			auto* _c = reg.try_get<GameCameraComponent>(e);
			if (!_c) return;
			if (!ImGui::CollapsingHeader("GameCamera", ImGuiTreeNodeFlags_DefaultOpen)) return;

			const char* modeNames[] = { "Follow", "LookAt" };
			int mode = (int)_c->mode;
			if (ImGui::Combo("Mode", &mode, modeNames, 2))
				_c->mode = (Mode)mode;

			// FollowTargetは自動で探す(Playerタグ)
			std::string followTargetName = "None (auto)";
			if (_c->followTarget != entt::null && reg.valid(_c->followTarget))
				followTargetName = "Entity " + std::to_string((uint32_t)_c->followTarget);

			ImGui::Text("Follow Target: %s", followTargetName.c_str());

			// LookAtTargetは自動で探す(Enemyタグ)
			std::string lookAtTargetName = "None (auto)";
			if (_c->lookAtTarget != entt::null && reg.valid(_c->lookAtTarget))
				lookAtTargetName = "Entity " + std::to_string((uint32_t)_c->lookAtTarget);

			ImGui::Text("LookAt Target: %s", lookAtTargetName.c_str());

			ImGui::DragFloat("Yaw", &_c->yaw, 0.01f);
			ImGui::DragFloat("Pitch", &_c->pitch, 0.01f, -1.5f, 1.5f);

			// --- Follow Camera ---
			ImGui::Separator();
			ImGui::Text("Follow Camera");
			ImGui::DragFloat("Distance", &_c->followCameraDistance, 0.1f, 0.0f, 50.0f);
			ImGui::DragFloat("Height", &_c->followCameraHeight, 0.1f, 0.0f, 20.0f);
			ImGui::DragFloat("Target Y Up", &_c->followCameraTargetYUp, 0.1f, 0.0f, 10.0f);
			ImGui::DragFloat("Eye Speed Ratio", &_c->followCameraEyeSpeedRatio, 0.5f, 0.0f, 100.0f);
			ImGui::DragFloat("Target Speed Ratio", &_c->followCameraTargetSpeedRatio, 0.5f, 0.0f, 100.0f);

			// --- LockOn Camera（共通） ---
			ImGui::Separator();
			ImGui::Text("LockOn Camera");
			ImGui::DragFloat("Eye Speed Ratio##LockOn", &_c->lockOnCameraEyeSpeedRatio, 0.5f, 0.0f, 100.0f);
			ImGui::DragFloat("Target Speed Ratio##LockOn", &_c->lockOnCameraTargetSpeedRatio, 0.5f, 0.0f, 100.0f);
			ImGui::DragFloat("Range Switch Distance", &_c->lockOnRangeSwitchDistance, 0.1f, 0.1f, 50.0f);

			// --- 円の中心O ---
			ImGui::Separator();
			ImGui::Text("Center O");
			ImGui::DragFloat("Center Ratio", &_c->lockOnCenterRatio, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat("Far Center Max Distance", &_c->lockOnFarCenterMaxDistance, 0.05f, 0.0f, 20.0f);
			ImGui::DragFloat("Target Circle Radius Max", &_c->lockOnTargetCircleRadiusMax, 0.1f, 0.0f, 20.0f);

			// --- 円周上の角度・スティック操作 ---
			ImGui::Separator();
			ImGui::Text("Orbit");
			ImGui::DragFloat("Orbit Rotate Speed", &_c->lockOnOrbitRotateSpeed, 0.1f, 0.0f, 20.0f);
			ImGui::DragFloat("Orbit Pitch Rotate Speed", &_c->lockOnOrbitPitchRotateSpeed, 0.1f, 0.0f, 20.0f);
			ImGui::DragFloat("Orbit Return Speed", &_c->lockOnOrbitReturnSpeed, 0.5f, 0.0f, 50.0f);
			ImGui::Separator();
			ImGui::Text("Orbit");
			ImGui::DragFloat("Orbit Angle", &_c->lockOnOrbitAngle, 0.01f);
			ImGui::DragFloat("Orbit Pitch", &_c->lockOnOrbitPitch, 0.01f, -0.8f, 0.8f);

			// --- くの字の向き ---
			ImGui::Separator();
			ImGui::Text("Side");
			ImGui::DragFloat("Side Switch Threshold", &_c->lockOnSideSwitchThreshold, 0.01f, 0.0f, 3.14f);
			ImGui::DragFloat("Side Smooth Speed", &_c->lockOnSideSmoothSpeed, 0.5f, 0.0f, 50.0f);
			ImGui::Text("Side : %.2f -> %.2f", _c->lockOnSide, _c->lockOnSideSmooth);

			// --- 近距離用パラメータ ---
			ImGui::Separator();
			ImGui::Text("Near");
			ImGui::DragFloat("Near Distance", &_c->lockOnNearCameraDistance, 0.1f, 0.0f, 30.0f);
			ImGui::DragFloat("Near Height", &_c->lockOnNearCameraHeight, 0.1f, 0.0f, 20.0f);
			ImGui::DragFloat("Near Target Y Up", &_c->lockOnNearCameraTargetYUp, 0.1f, 0.0f, 10.0f);
			ImGui::DragFloat("Near Angle Offset Max", &_c->lockOnNearAngleOffsetMax, 0.5f, 0.0f, 90.0f);

			// --- 遠距離用パラメータ ---
			ImGui::Separator();
			ImGui::Text("Far");
			ImGui::DragFloat("Far Distance", &_c->lockOnFarCameraDistance, 0.1f, 0.0f, 30.0f);
			ImGui::DragFloat("Far Height", &_c->lockOnFarCameraHeight, 0.1f, 0.0f, 20.0f);
			ImGui::DragFloat("Far Target Y Up", &_c->lockOnFarCameraTargetYUp, 0.1f, 0.0f, 10.0f);

			// --- 減衰ばね ---
			ImGui::Separator();
			ImGui::Text("Spring");
			ImGui::DragFloat("Stiffness", &_c->eyeSpringStiffness, 5.0f, 0.0f, 1000.0f);
			ImGui::DragFloat("Damping", &_c->eyeSpringDamping, 1.0f, 0.0f, 200.0f);

			// --- 現在のeye/target（デバッグ表示） ---
			ImGui::Separator();
			ImGui::DragFloat3("Eye", &_c->base.currentEye.x, 0.1f);
			ImGui::DragFloat3("Target", &_c->base.currentTarget.x, 0.1f);
		});

	// コントローラーコンポーネントの登録
	REGISTER_COMPONENT(ControllerComponent, "Controller",
		DRAW_COMPONENT(ControllerComponent, "Controller",
			{
				if (_c->connected)
				{
					const char* typeName = _c->type == ControllerType::PS5 ? "PS5 DualSense" : "Xbox";
					ImGui::TextColored({ 0.3f, 1.0f, 0.3f, 1.0f }, u8"[%s] 接続中", typeName);
				}
				else
				{
					ImGui::TextColored({ 0.6f, 0.6f, 0.6f, 1.0f }, u8"未接続");
				}
			}));


	// 物理タイプコンポーネントの登録
	ComponentRegistry::Register("Physics",
		{
			"Physics",
			// Add: PhysicsBodyTypeComponentのみ付与(種別未選択状態)
			[](auto& reg, auto e)
			{
				if (!reg.any_of<PhysicsBodyTypeComponent>(e))
					reg.emplace<PhysicsBodyTypeComponent>(e);
			},
		// Remove: 共通コンポーネントと紐づいた個別のコンポーネントを両方削除
		[](auto& reg, auto e)
		{
			if (reg.any_of<StaticBodyComponent>(e))
				reg.remove<StaticBodyComponent>(e);

			if (auto* ctrl = reg.try_get<CharacterControllerComponent>(e))
			{
				PhysicsDestroyQueue::Push(ctrl->controller);
				reg.remove<CharacterControllerComponent>(e);
			}

			if (reg.any_of<MovementComponent>(e))
				reg.remove<MovementComponent>(e);

			reg.remove<PhysicsBodyTypeComponent>(e);
		},
		// Has
		[](auto& reg, auto e) { return reg.any_of<PhysicsBodyTypeComponent>(e); },
		// GetMeta
		[](entt::registry& r, entt::entity e) -> entt::meta_any
		{
			auto* c = r.try_get<PhysicsBodyTypeComponent>(e);
			if (!c) return entt::meta_any{};
			return entt::forward_as_meta(*c);
		}
		});
	ComponentInspectorRegistry::Register("Physics",
		[](auto& reg, auto e)
		{
			auto* body = reg.try_get<PhysicsBodyTypeComponent>(e);
			if (!body) return;
			if (!ImGui::CollapsingHeader("Physics", ImGuiTreeNodeFlags_DefaultOpen)) return;

			const char* typeNames[] = { "None", "Static", "Dynamic", "Character" };
			int current = (int)body->type;

			if (ImGui::Combo("Type", &current, typeNames, 4))
			{
				auto newType = (PhysicsBodyTypeComponent::Type)current;

				if (body->type == PhysicsBodyTypeComponent::Type::Static)
				{
					auto* staticBody = reg.try_get<StaticBodyComponent>(e);
					if (staticBody && staticBody->actor)
					{
						PhysicsDestroyQueue::Push(staticBody->actor);
					}
					reg.remove<StaticBodyComponent>(e);
				}
				else if (body->type == PhysicsBodyTypeComponent::Type::Character)
				{
					auto* ctrl = reg.try_get<CharacterControllerComponent>(e);
					if (ctrl && ctrl->controller)
					{
						PhysicsDestroyQueue::Push(ctrl->controller);
					}
					reg.remove<CharacterControllerComponent>(e);

					if (reg.any_of<MovementComponent>(e))
						reg.remove<MovementComponent>(e);
				}

				body->type = newType;

				if (newType == PhysicsBodyTypeComponent::Type::Static)
				{
					reg.emplace<StaticBodyComponent>(e);
				}
				else if (newType == PhysicsBodyTypeComponent::Type::Character)
				{
					reg.emplace<CharacterControllerComponent>(e);

					if (!reg.any_of<MovementComponent>(e))
						reg.emplace<MovementComponent>(e);
				}
			}

			switch (body->type)
			{
			case PhysicsBodyTypeComponent::Type::Static:
			{
				auto* staticBody = reg.try_get<StaticBodyComponent>(e);
				if (staticBody)
				{
					ImGui::Text(staticBody->actor ? u8"状態: 生成済み" : u8"状態: 未生成");
				}
				break;
			}
			case PhysicsBodyTypeComponent::Type::Dynamic:
				ImGui::TextColored({ 1, 0.5f, 0, 1 }, u8"Dynamicは未実装です");
				break;
			case PhysicsBodyTypeComponent::Type::Character:
			{
				auto* ctrl = reg.try_get<CharacterControllerComponent>(e);
				if (ctrl)
				{
					bool created = (ctrl->controller != nullptr);
					ImGui::Text(created ? u8"状態: 生成済み" : u8"状態: 未生成");
				}
				break;
			}
			default:
				break;
			}
		});

		// IKコンポーネントの登録
		REGISTER_COMPONENT(IKComponent, "IK",
			[](auto& reg, auto e)
			{
				auto* ik = reg.try_get<IKComponent>(e);
				auto* skinned = reg.try_get<SkinnedMeshComponent>(e);
				if (!ik) return;
				if (!ImGui::CollapsingHeader("IK", ImGuiTreeNodeFlags_DefaultOpen)) return;
				if (!skinned || !skinned->model)
				{
					ImGui::TextColored({ 1, 0.5f, 0, 1 }, "No SkinnedMesh found.");
					return;
				}
				ImGui::Checkbox("Enabled", &ik->enabled);
				int removeIndex = -1; // このフレームで削除したいfootのインデックス(-1なら削除なし)
				for (size_t i = 0; i < ik->feet.size(); ++i)
				{
					ImGui::PushID((int)i);
					std::string label = "Foot " + std::to_string(i);
					if (ImGui::TreeNode(label.c_str()))
					{
						FootIKSettings& foot = ik->feet[i];
						auto& nodes = skinned->model->GetNodes();
						auto drawBoneCombo = [&](const char* comboLabel, int& boneIndex)
							{
								const char* previewName = (boneIndex >= 0 && boneIndex < (int)nodes.size())
									? nodes[boneIndex].name.c_str()
									: "(Invalid Index)";

								if (ImGui::BeginCombo(comboLabel, previewName))
								{
									for (int n = 0; n < (int)nodes.size(); ++n)
									{
										bool isSelected = (boneIndex == n);
										if (ImGui::Selectable(nodes[n].name.c_str(), isSelected))
										{
											boneIndex = n;
										}
										if (isSelected)
										{
											ImGui::SetItemDefaultFocus();
										}
									}
									ImGui::EndCombo();
								}
							};
						drawBoneCombo("Root Bone", foot.rootBoneIndex);
						drawBoneCombo("Mid Bone", foot.midBoneIndex);
						drawBoneCombo("End Bone", foot.endBoneIndex);
						ImGui::DragFloat("Ray Start Offset", &foot.rayStartOffset, 0.01f);
						ImGui::DragFloat("Ray Max Distance", &foot.rayMaxDistance, 0.01f);
						ImGui::DragFloat("Ankle Height", &foot.ankleHeight, 0.01f);
						ImGui::Checkbox("Pole Vector Forward (Z-)", &foot.poleVectorForward);
						ImGui::DragFloat3("Target Pos", &foot.targetPos.x, 0.01f);
						if (ImGui::Button("Reset Target Pos"))
						{
							foot.hasInitializedTarget = false;
						}
						if (ImGui::Button("Remove This Foot"))
						{
							removeIndex = (int)i;
						}
						ImGui::TreePop();
					}
					ImGui::PopID();
				}
				if (removeIndex >= 0)
				{
					ik->feet.erase(ik->feet.begin() + removeIndex);
				}
				if (ImGui::Button("Add Foot"))
				{
					ik->feet.push_back(FootIKSettings{});
				}
			}); 


		// シーケンサーコンポーネントの登録
		REGISTER_COMPONENT_AUTO(SequencerComponent, "Sequencer");

		// アニメーション速度コンポーネントの登録
		REGISTER_COMPONENT_AUTO(AnimationSpeedComponent, "Animation Speed"); 
}
