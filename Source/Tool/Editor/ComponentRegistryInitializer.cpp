#include "ComponentRegistryInitializer.h"
#include "Tool/Editor/ComponentRegistry.h"
#include <imgui.h>
#include <Windows.h>
#include <commdlg.h>
#include <filesystem>
#include <algorithm>

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
        DrawFunc,                                                                   \
        [](entt::registry& r, entt::entity e) -> entt::meta_any                     \
        {                                                                           \
            auto* c = r.try_get<Type>(e);                                           \
            if (!c) return entt::meta_any{};                                        \
            return entt::forward_as_meta(*c);                                       \
        }                                                                           \
    });                                                                             \
    ComponentRegistry::RegisterTypeName(#Type, Name)

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
	// トランスフォームコンポーネントの登録
	REGISTER_COMPONENT(TransformComponent, "Transform",
		[](auto& reg, auto e)
		{
			auto* _c = reg.try_get<TransformComponent>(e);
			if (!_c) return;

			if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
			{
				MetaRegistry::DrawMetaFields(entt::resolve<TransformComponent>(), entt::forward_as_meta(*_c));
			}
		});

	// メッシュコンポーネントの登録
	REGISTER_COMPONENT(MeshComponent, "Mesh",
		[](auto& reg, auto e)
		{
			auto* _c = reg.try_get<MeshComponent>(e);
			if (!_c) return;

			if (ImGui::CollapsingHeader("Mesh", ImGuiTreeNodeFlags_DefaultOpen))
			{
				MetaRegistry::DrawMetaFields(entt::resolve<MeshComponent>(), entt::forward_as_meta(*_c));
			}
		});

	// マテリアルコンポーネントの登録
	ComponentRegistry::Register("Material",
		{
			"Material",
			[](auto& reg, auto e) { if (!reg.any_of<MaterialComponent>(e)) reg.emplace<MaterialComponent>(e); },
			[](auto& reg, auto e) { reg.remove<MaterialComponent>(e); },
			[](auto& reg, auto e) { return reg.any_of<MaterialComponent>(e); },
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
			}
		});

	// Player待機コンポーネントの登録
	REGISTER_COMPONENT(PlayerIdleComponent, "Player Idle",
		[](auto& reg, auto e)
		{
			auto* _c = reg.try_get<PlayerIdleComponent>(e);
			if (!_c) return;

			if (ImGui::CollapsingHeader("Player Idle", ImGuiTreeNodeFlags_DefaultOpen))
			{
				MetaRegistry::DrawMetaFields(entt::resolve<PlayerIdleComponent>(), entt::forward_as_meta(*_c));
			}
		});

	// Player移動コンポーネントの登録
	REGISTER_COMPONENT(PlayerMoveComponent, "Player Move",
		[](auto& reg, auto e)
		{
			auto* _c = reg.try_get<PlayerMoveComponent>(e);
			if (!_c) return;

			if (ImGui::CollapsingHeader("Player Move", ImGuiTreeNodeFlags_DefaultOpen))
			{
				MetaRegistry::DrawMetaFields(entt::resolve<PlayerMoveComponent>(), entt::forward_as_meta(*_c));
			}
		});
	// Dodgeコンポーネントの登録
	REGISTER_COMPONENT(PlayerDodgeComponent, "Player Dodge",
		[](auto& reg, auto e)
		{
			auto* _c = reg.try_get<PlayerDodgeComponent>(e);
			if (!_c) return;

			if (ImGui::CollapsingHeader("Player Dodge", ImGuiTreeNodeFlags_DefaultOpen))
			{
				MetaRegistry::DrawMetaFields(entt::resolve<PlayerDodgeComponent>(), entt::forward_as_meta(*_c));
			}
		});

	// Sprintコンポーネントの登録
	REGISTER_COMPONENT(PlayerSprintComponent, "Player Sprint",
		[](auto& reg, auto e)
		{
			auto* _c = reg.try_get<PlayerSprintComponent>(e);
			if (!_c) return;

			if (ImGui::CollapsingHeader("Player Sprint", ImGuiTreeNodeFlags_DefaultOpen))
			{
				MetaRegistry::DrawMetaFields(entt::resolve<PlayerSprintComponent>(), entt::forward_as_meta(*_c));
			}
		});

	// インプットコンポーネントの登録
	REGISTER_COMPONENT(GameInputComponent, "GameInput",
		[](auto& reg, auto e)
		{
			auto* _c = reg.try_get<GameInputComponent>(e);
			if (!_c) return;

			if (ImGui::CollapsingHeader("Input", ImGuiTreeNodeFlags_DefaultOpen))
			{
				MetaRegistry::DrawMetaFields(entt::resolve<GameInputComponent>(), entt::forward_as_meta(*_c));
			}
		});

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
			[](auto& reg, auto e)
			{
				auto* _c = reg.try_get<GameCameraComponent>(e);
				if (!_c) return;
				if (!ImGui::CollapsingHeader("GameCamera", ImGuiTreeNodeFlags_DefaultOpen)) return;

				const char* modeNames[] = { "Follow", "LookAt", "Fixed" };
				int mode = (int)_c->mode;
				if (ImGui::Combo("Mode", &mode, modeNames, 3))
					_c->mode = (GameCameraComponent::Mode)mode;

				std::string targetName = "None";
				if (_c->target != entt::null && reg.valid(_c->target))
					targetName = "Entity " + std::to_string((uint32_t)_c->target);

				ImGui::Text("Target"); ImGui::SameLine();
				ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 60.0f);
				ImGui::InputText("##target", targetName.data(),
					targetName.size() + 1, ImGuiInputTextFlags_ReadOnly);

				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY"))
						_c->target = *(const entt::entity*)payload->Data;
					ImGui::EndDragDropTarget();
				}
				ImGui::SameLine();
				if (ImGui::Button("Clear")) _c->target = entt::null;

				ImGui::DragFloat("Distance",     &_c->distance,    0.1f, 0.0f, 50.0f);
				ImGui::DragFloat("Height",       &_c->height,      0.1f, 0.0f, 20.0f);
				ImGui::DragFloat("Follow Speed", &_c->followSpeed, 0.1f, 0.1f, 30.0f);
				ImGui::DragFloat("Pitch",        &_c->pitch,       0.01f, -1.5f, 1.5f);
				ImGui::Separator();
				ImGui::DragFloat3("Eye",    &_c->base.currentEye.x,    0.1f);
				ImGui::DragFloat3("Target", &_c->base.currentTarget.x, 0.1f);
			}
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
					// これは1フレーム遅延させ、他の更新の内でまとめて処理する
					PhysicsDestroyQueue::Push(ctrl->controller);
					reg.remove<CharacterControllerComponent>(e);
				}

				// CharacterControllerとセットで生まれたMovementComponentも一緒に外す
				if (reg.any_of<MovementComponent>(e))
					reg.remove<MovementComponent>(e);

				reg.remove<PhysicsBodyTypeComponent>(e);		},
		// Has
		[](auto& reg, auto e) { return reg.any_of<PhysicsBodyTypeComponent>(e); },
		// DrawImGui
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

						// 切り替え前の実体があれば削除予約してから外す
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

							// CharacterControllerとセットで生まれたMovementComponentも一緒に外す
							if (reg.any_of<MovementComponent>(e))
								reg.remove<MovementComponent>(e);
						}

						body->type = newType;

						// 切り替え後の実体を新規作成
						if (newType == PhysicsBodyTypeComponent::Type::Static)
						{
							reg.emplace<StaticBodyComponent>(e);
						}
						else if (newType == PhysicsBodyTypeComponent::Type::Character)
						{
							reg.emplace<CharacterControllerComponent>(e);

							// CharacterControllerとセットでMovementComponentも生成
							if (!reg.any_of<MovementComponent>(e))
								reg.emplace<MovementComponent>(e);
						}
					}

					// 選択中の種別ごとの項目表示
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
		REGISTER_COMPONENT(SequencerComponent, "Sequencer",
			[](auto& reg, auto e)
			{
				auto* seq = reg.try_get<SequencerComponent>(e);
				if (!seq) return;

				if (ImGui::CollapsingHeader("Sequencer", ImGuiTreeNodeFlags_DefaultOpen))
				{
					MetaRegistry::DrawMetaFields(entt::resolve<SequencerComponent>(), entt::forward_as_meta(*seq));
				}
			}); 

		// アニメーション速度コンポーネントの登録
		REGISTER_COMPONENT(AnimationSpeedComponent, "Animation Speed",
			[](auto& reg, auto e)
			{
				auto* animSpeed = reg.try_get<AnimationSpeedComponent>(e);
				if (!animSpeed) return;

				if (ImGui::CollapsingHeader("Animation Speed", ImGuiTreeNodeFlags_DefaultOpen))
				{
					MetaRegistry::DrawMetaFields(entt::resolve<AnimationSpeedComponent>(), entt::forward_as_meta(*animSpeed));
				}
			});
}
