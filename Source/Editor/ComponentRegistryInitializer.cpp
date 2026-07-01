#include "ComponentRegistryInitializer.h"
#include "Editor/ComponentRegistry.h"
#include <imgui.h>
#include <Windows.h>
#include <commdlg.h>
#include <filesystem>
#include <algorithm>

#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/MeshComponent.h"
#include "ECS/Components/MaterialComponent.h"

#include "ECS/Components/InputComponent.h"
#include "ECS/Components/ControllerComponent.h"
#include "ECS/Components/MovementComponent.h"

#include "ECS/Components/AnimatorComponent.h"
#include "ECS/Components/SkinnedMeshComponent.h"

#include "ECS/Components/FreeCameraComponent.h"
#include "ECS/Components/GameCameraComponent.h"

#include "Core/PhysicsDestroyQueue.h"
#include "ECS/Components/PhysicsComponents.h"

#include "ECS/Components/IKComponent.h"

#define REGISTER_COMPONENT(Type, Name, DrawFunc)                                    \
    ComponentRegistry::Register(Name,                                               \
    {                                                                               \
        Name,                                                                       \
        [](auto& r, auto e){ if (!r.any_of<Type>(e)) r.emplace<Type>(e); },         \
        [](auto& r, auto e){ r.remove<Type>(e); },                                  \
        [](auto& r, auto e){ return r.any_of<Type>(e); },                           \
        DrawFunc                                                                    \
    })

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
		DRAW_COMPONENT(TransformComponent, "Transform",
			{
				ImGui::DragFloat3("Position", &_c->position.x, 0.1f);
				ImGui::DragFloat3("Rotation", &_c->rotation.x, 0.01f);
				ImGui::DragFloat3("Scale",    &_c->scale.x,    0.01f);
			}));

	// メッシュコンポーネントの登録
	REGISTER_COMPONENT(MeshComponent, "Mesh",
		DRAW_COMPONENT(MeshComponent, "Mesh",
			{
				std::string displayName = _c->modelPath.empty()
					? "None"
					: std::filesystem::path(_c->modelPath).stem().string();

				ImGui::InputText("##modelName", displayName.data(),
					displayName.size() + 1, ImGuiInputTextFlags_ReadOnly);
				ImGui::SameLine();

				if (ImGui::Button("...##openModel"))
				{
					char currentDir[256] = {};
					GetCurrentDirectoryA(sizeof(currentDir), currentDir);
					std::string dataDir = std::string(currentDir) + "\\Data";

					OPENFILENAMEA ofn = {};
					char filePath[256] = {};
					ofn.lStructSize = sizeof(ofn);
					ofn.lpstrFilter = u8"GLTFファイル\0*.gltf;*.glb\0すべてのファイル\0*.*\0";
					ofn.lpstrFile = filePath;
					ofn.nMaxFile = sizeof(filePath);
					ofn.lpstrTitle = u8"モデルを選択";
					ofn.lpstrInitialDir = dataDir.c_str();
					ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

					if (GetOpenFileNameA(&ofn))
					{
						std::string absPath = filePath;
						std::string basePath = std::string(currentDir) + "\\";
						if (absPath.find(basePath) == 0)
							absPath = absPath.substr(basePath.size());
						std::replace(absPath.begin(), absPath.end(), '\\', '/');
						_c->modelPath = absPath;
					}
				}
			}));

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

	// ムーブメントコンポーネントの登録
	REGISTER_COMPONENT(MovementComponent, "Movement",
		DRAW_COMPONENT(MovementComponent, "Movement",
			{
				ImGui::DragFloat("Move Speed", &_c->moveSpeed, 0.1f,   0.0f,   20.0f);
				ImGui::DragFloat("Jump Power", &_c->jumpPower, 0.1f,   0.0f,   50.0f);
				ImGui::DragFloat("Gravity",    &_c->gravity,   0.1f, -100.0f,   0.0f);
				ImGui::DragFloat3("Velocity",  &_c->velocity.x, 0.01f);
				ImGui::Checkbox("Is Grounded", &_c->isGrounded);
			}));

	// インプットコンポーネントの登録
	REGISTER_COMPONENT(InputComponent, "Input",
		DRAW_COMPONENT(InputComponent, "Input",
			{
				ImGui::Text("MoveX:  %.2f", _c->moveX);
				ImGui::Text("MoveZ:  %.2f", _c->moveZ);
				ImGui::Text(u8"Jump:   %s", _c->jump ? "true" : "false");
				ImGui::Text(u8"Attack: %s", _c->attack ? "true" : "false");
			}));

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

			ImGui::SliderFloat("Play Speed", &anim->playSpeed, 0.0f, 3.0f);
			ImGui::Checkbox("Loop", &anim->loop);

			if (anim->isPlaying)
			{
				if (ImGui::Button("Stop")) anim->isPlaying = false;
			}
			else
			{
				if (ImGui::Button("Play")) { anim->isPlaying = true; anim->isEnd = false; }
			}
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
		// Remove: 種別コンポーネントと紐づく実体コンポーネントを両方削除
		[](auto& reg, auto e)
		{
			if (reg.any_of<StaticBodyComponent>(e))
				reg.remove<StaticBodyComponent>(e);

			if (auto* ctrl = reg.try_get<CharacterControllerComponent>(e))
			{
				// 解放を1フレーム遅延させ、次の物理更新の頭でまとめて処理する
				PhysicsDestroyQueue::Push(ctrl->controller);
				reg.remove<CharacterControllerComponent>(e);
			}

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

						// 切り替え前の実体があれば削除予約してからComponentを外す
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

				// 有効フラグ
				ImGui::Checkbox("Enabled", &ik->enabled);

				// SkinnedMeshがなければ警告
				if (!skinned || !skinned->model)
				{
					ImGui::TextColored({ 1, 0.5f, 0, 1 }, "No SkinnedMesh found.");
					return;
				}

				const auto& nodes = skinned->model->GetNodes();
				if (nodes.empty())
				{
					ImGui::TextColored({ 1, 0.5f, 0, 1 }, "No bones found.");
					return;
				}

				// ボーン名コンボ
				if (ImGui::BeginCombo("Target Bone", ik->targetBoneName.c_str()))
				{
					for (const auto& node : nodes)
					{
						bool selected = (ik->targetBoneName == node.name);
						if (ImGui::Selectable(node.name.c_str(), selected))
						{
							ik->targetBoneName = node.name;
						}
						if (selected) ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}

				ImGui::Separator();

				// 位置オフセット・回転(オイラー角)
				ImGui::DragFloat3("Position Offset", &ik->manualPosition.x, 0.01f);
				ImGui::DragFloat3("Rotation (Euler)", &ik->manualRotationEuler.x, 0.01f);
			});
}