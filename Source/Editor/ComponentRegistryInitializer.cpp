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

#include "Core/ShaderPassLibrary.h"

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

    // マテリアルコンポーネントの登録
    REGISTER_COMPONENT(MaterialComponent, "Material",
        [](auto& reg, auto e)
        {
            auto* mat = reg.try_get<MaterialComponent>(e);
            if (!mat) return;
            if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen))
            {
                for (const auto& pass : mat->passes)
                {
                    ImGui::Text("Shader: %s", pass.name.c_str());
                }
            }
        });

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
}