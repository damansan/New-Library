#include "ComponentRegistryInitializer.h"
#include "Tool/Editor/ComponentInspectorRegistry.h"

#include <imgui.h>
#include <Windows.h>
#include <commdlg.h>
#include <filesystem>
#include <algorithm>

#include "Engine/ECS/Components/ControllerComponent.h"
#include "Engine/ECS/Components/MovementComponent.h"
#include "Engine/ECS/Components/AnimatorComponent.h"
#include "Engine/ECS/Components/SkinnedMeshComponent.h"
#include "Engine/ECS/Components/FreeCameraComponent.h"
#include "Engine/ECS/Components/PhysicsComponents.h"
#include "Engine/ECS/Components/IKComponent.h"

#include "MetaRegistry.h"

#define DRAW_COMPONENT(Type, Label, body)                                   \
    [](auto& reg, auto e)                                                   \
    {                                                                       \
        auto* _c = reg.try_get<Type>(e);                                    \
        if (!_c) return;                                                    \
        if (ImGui::CollapsingHeader(Label, ImGuiTreeNodeFlags_DefaultOpen)) \
        { body }                                                            \
    }

void ComponentRegistryInitializer::Initialize()
{
	// アニメーター
	REGISTER_INSPECTOR(AnimatorComponent,
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

	// フリーカメラ
	REGISTER_INSPECTOR(FreeCameraComponent,
		DRAW_COMPONENT(FreeCameraComponent, "FreeCamera",
			{
				ImGui::DragFloat("Camera Speed", &_c->cameraSpeed, 0.1f, 0.1f, 100.0f);
				ImGui::DragFloat("Yaw",          &_c->yaw,         0.01f);
				ImGui::DragFloat("Pitch",        &_c->pitch,       0.01f, -1.5f, 1.5f);
				ImGui::Separator();
				ImGui::DragFloat3("Eye",    &_c->base.currentEye.x,    0.1f);
				ImGui::DragFloat3("Target", &_c->base.currentTarget.x, 0.1f);
			}));

	// コントローラー
	REGISTER_INSPECTOR(ControllerComponent,
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

	// 物理(実行時の登録はEngineComponentsで行う。ここは描画のみ)
	REGISTER_INSPECTOR(PhysicsBodyTypeComponent,
		[](auto& reg, auto e)
		{
			auto* body = reg.try_get<PhysicsBodyTypeComponent>(e);
			if (!body) return;
			if (!ImGui::CollapsingHeader("Physics", ImGuiTreeNodeFlags_DefaultOpen)) return;

			// typeを書き換えるだけ。剛体の生成・破棄は、PhysicsSystemが追従して行う
			ImGui::PushID("fields");
			MetaRegistry::DrawMetaFields(entt::resolve<PhysicsBodyTypeComponent>(), entt::forward_as_meta(*body));
			ImGui::PopID();

			switch (body->type)
			{
			case PhysicsBodyType::Static:
			{
				auto* staticBody = reg.try_get<StaticBodyComponent>(e);
				if (staticBody)
				{
					ImGui::Text(staticBody->actor ? u8"状態: 生成済み" : u8"状態: 未生成");
				}
				break;
			}
			case PhysicsBodyType::Dynamic:
				ImGui::TextColored({ 1, 0.5f, 0, 1 }, u8"Dynamicは未実装です");
				break;
			case PhysicsBodyType::Character:
			{
				auto* ctrl = reg.try_get<CharacterControllerComponent>(e);
				if (ctrl)
				{
					bool created = (ctrl->controller != nullptr);
					ImGui::Text(created ? u8"状態: 生成済み" : u8"状態: 未生成");
				}

				// 移動の状態（Characterと一緒に付与されるMovementComponent）
				auto* movement = reg.try_get<MovementComponent>(e);
				if (movement && ImGui::TreeNode("Movement"))
				{
					MetaRegistry::DrawMetaFields(entt::resolve<MovementComponent>(), entt::forward_as_meta(*movement));
					ImGui::TreePop();
				}
				break;
			}

			default:
				break;
			}
		});

	// IK
	REGISTER_INSPECTOR(IKComponent,
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
}