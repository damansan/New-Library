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
#include "ECS/Components/MovementComponent.h"
#include "ECS/Components/AnimatorComponent.h"
#include "ECS/Components/SkinnedMeshComponent.h"
#include "ECS/Components/CameraComponent.h"


void ComponentRegistryInitializer::Initialize()
{
    // トランスフォームコンポーネントの登録
    REGISTER_COMPONENT(TransformComponent, "Transform",
        [](auto& reg, auto e)
        {
            auto* t = reg.try_get<TransformComponent>(e);
            if (!t) return;
            if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::DragFloat3("Position", &t->position.x, 0.1f);
                ImGui::DragFloat3("Rotation", &t->rotation.x, 0.01f);
                ImGui::DragFloat3("Scale", &t->scale.x, 0.01f);
            }
        });

    // メッシュコンポーネントの登録
    REGISTER_COMPONENT(MeshComponent, "Mesh",
        [](auto& reg, auto e)
        {
            auto* m = reg.try_get<MeshComponent>(e);
            if (!m) return;
            if (ImGui::CollapsingHeader("Mesh", ImGuiTreeNodeFlags_DefaultOpen))
            {
                // モデル名だけ表示（フルパスからファイル名を取得）
                std::string displayName = m->modelPath.empty()
                    ? "None"
                    : std::filesystem::path(m->modelPath).stem().string();

                ImGui::InputText("##modelName", displayName.data(),
                    displayName.size() + 1,
                    ImGuiInputTextFlags_ReadOnly);

                ImGui::SameLine();

                if (ImGui::Button("...##openModel"))
                {
                    // Data フォルダの絶対パスを取得
                    char currentDir[256] = {};
                    GetCurrentDirectoryA(sizeof(currentDir), currentDir);
                    std::string dataDir = std::string(currentDir) + "\\Data";

                    OPENFILENAMEA ofn = {};
                    char filePath[256] = {};

                    ofn.lStructSize = sizeof(ofn);
                    ofn.hwndOwner = nullptr;
                    ofn.lpstrFilter = "GLTFファイル\0*.gltf;*.glb\0すべてのファイル\0*.*\0";
                    ofn.lpstrFile = filePath;
                    ofn.nMaxFile = sizeof(filePath);
                    ofn.lpstrTitle = "モデルを選択";
                    ofn.lpstrInitialDir = dataDir.c_str(); // Data から開く
                    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

                    if (GetOpenFileNameA(&ofn))
                    {
                        // 絶対パスを相対パスに変換
                        std::string absPath = filePath;
                        std::string basePath = std::string(currentDir) + "\\";

                        // basePath からの相対パスに変換
                        if (absPath.find(basePath) == 0)
                        {
                            absPath = absPath.substr(basePath.size());
                        }

                        // バックスラッシュをスラッシュに統一
                        std::replace(absPath.begin(), absPath.end(), '\\', '/');
                        m->modelPath = absPath;
                    }
                }
            }
        });

    //// マテリアルコンポーネントの登録
    //REGISTER_COMPONENT(MaterialComponent, "Material",
    //    [](auto& reg, auto e)
    //    {
    //        auto* mat = reg.try_get<MaterialComponent>(e);
    //        if (!mat) return;
    //        if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen))
    //        {
    //            for (const auto& pass : mat->passes)
    //            {
    //                ImGui::Text("Shader: %s", pass.name.c_str());
    //            }
    //        }
    //    });

    // インプットコンポーネントの登録
    REGISTER_COMPONENT(InputComponent, "Input",
        [](auto& reg, auto e)
        {
            auto* input = reg.try_get<InputComponent>(e);
            if (!input) return;
            if (ImGui::CollapsingHeader("Input", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Text("MoveX:  %.2f", input->moveX);
                ImGui::Text("MoveZ:  %.2f", input->moveZ);
                ImGui::Text("Jump:   %s", input->jump ? "true" : "false");
                ImGui::Text("Attack: %s", input->attack ? "true" : "false");
            }
        });

    // ムーブメントコンポーネントの登録
    REGISTER_COMPONENT(MovementComponent, "Movement",
        [](auto& reg, auto e)
        {
            auto* mv = reg.try_get<MovementComponent>(e);
            if (!mv) return;
            if (ImGui::CollapsingHeader("Movement", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::DragFloat("Move Speed", &mv->moveSpeed, 0.1f, 0.0f, 20.0f);
                ImGui::DragFloat("Jump Power", &mv->jumpPower, 0.1f, 0.0f, 50.0f);
                ImGui::DragFloat("Gravity", &mv->gravity, 0.1f, -100.0f, 0.0f);
                ImGui::DragFloat3("Velocity", &mv->velocity.x, 0.01f);
                ImGui::Checkbox("Is Grounded", &mv->isGrounded);
            }
        });

    // アニメーターコンポーネントの登録
    REGISTER_COMPONENT(AnimatorComponent, "Animator",
        [](auto& reg, auto e)
        {
            auto* anim = reg.try_get<AnimatorComponent>(e);
            if (!anim) return;

            // SkinnedMeshComponent からアニメーション一覧を取得
            auto* skinned = reg.try_get<SkinnedMeshComponent>(e);

            if (ImGui::CollapsingHeader("Animator", ImGuiTreeNodeFlags_DefaultOpen))
            {
                // モデルがなければ警告表示
                if (!skinned || !skinned->model)
                {
                    ImGui::TextColored(
                        ImVec4(1, 0.5f, 0, 1),
                        "No SkinnedMesh found.");
                    return;
                }

                const auto& animations = skinned->model->GetAnimations();

                if (animations.empty())
                {
                    ImGui::TextColored(
                        ImVec4(1, 0.5f, 0, 1),
                        "No animations found.");
                    return;
                }

                // アニメーション選択ドロップダウン
                if (ImGui::BeginCombo(
                    "Animation",
                    anim->currentAnimation.c_str()))
                {
                    for (const auto& animation : animations)
                    {
                        bool isSelected =
                            (anim->currentAnimation == animation.name);

                        if (ImGui::Selectable(
                            animation.name.c_str(), isSelected))
                        {
                            anim->currentAnimation = animation.name;
                            anim->animationTime = 0.0f;
                            anim->isPlaying = true;
                            anim->isEnd = false;
                        }
                        if (isSelected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }

                // 再生速度
                ImGui::SliderFloat("Play Speed", &anim->playSpeed, 0.0f, 3.0f);

                // ループ
                ImGui::Checkbox("Loop", &anim->loop);

                // 再生/停止ボタン
                if (anim->isPlaying)
                {
                    if (ImGui::Button("Stop"))
                        anim->isPlaying = false;
                }
                else
                {
                    if (ImGui::Button("Play"))
                    {
                        anim->isPlaying = true;
                        anim->isEnd = false;
                    }
                }
            }
        });

    // カメラコンポーネントの登録
    ComponentRegistry::Register("Camera",
        {
            "Camera",
            [](auto& reg, auto e)
            {
                if (reg.any_of<CameraComponent>(e)) return;
                reg.emplace<CameraComponent>(e);
            },
            [](auto& reg, auto e) { reg.remove<CameraComponent>(e); },
            [](auto& reg, auto e) { return reg.any_of<CameraComponent>(e); },
            [](auto& reg, auto e)
            {
                auto* cam = reg.try_get<CameraComponent>(e);
                if (!cam) return;

                if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    // モード選択
                    const char* modeNames[] = { "Follow", "LookAt", "Fixed" };
                    int currentMode = (int)cam->mode;
                    if (ImGui::Combo("Mode", &currentMode, modeNames, 3))
                    {
                        cam->mode = (CameraComponent::Mode)currentMode;
                    }
                    
                    // ラベル表示
                    ImGui::Text("Target");
                    ImGui::SameLine();

                    // 追従対象表示
                    std::string targetName = "None";
                    if (cam->target != entt::null && reg.valid(cam->target))
                    {
                        targetName = "Entity " + std::to_string((uint32_t)cam->target);
                    }

                    // 表示用テキストボックス
                    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 60.0f);
                    ImGui::InputText("##target", targetName.data(),
                        targetName.size() + 1,
                        ImGuiInputTextFlags_ReadOnly);
                    
                    // InputText の直後にドロップターゲットを設定
                    if (ImGui::BeginDragDropTarget())
                    {
                        if (const ImGuiPayload* payload =
                            ImGui::AcceptDragDropPayload("ENTITY"))
                        {
                            entt::entity dropped =
                                *(const entt::entity*)payload->Data;
                            cam->target = dropped;
                        }
                        ImGui::EndDragDropTarget();
                    }

                    ImGui::SameLine();

                    // クリアボタン
                    if (ImGui::Button("Clear"))
                    {
                        cam->target = entt::null;
                    }

                    // パラメータ
                    ImGui::DragFloat("Distance", &cam->distance, 0.1f, 0.0f, 50.0f);
                    ImGui::DragFloat("Height", &cam->height, 0.1f, 0.0f, 20.0f);
                    ImGui::DragFloat("Follow Speed", &cam->followSpeed, 0.1f, 0.1f, 30.0f);
                    ImGui::DragFloat("Pitch", &cam->pitch, 0.01f, -1.5f, 1.5f);
                }
            }
        });
}