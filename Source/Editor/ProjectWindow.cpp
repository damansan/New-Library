#include "ProjectWindow.h"
#include <imgui.h>

#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/MeshComponent.h"
#include "ECS/Components/MaterialComponent.h"
#include <algorithm>

void ProjectWindow::Initialize(const std::string& rootPath)
{
    this->rootPath = rootPath;
}

void ProjectWindow::Draw(entt::registry& registry, EditorContext& context)
{
    ImGui::Begin("Project");

    if (rootPath.empty())
    {
        ImGui::Text("Root path not set.");
        ImGui::End();
        return;
    }

    // ルートパスからツリーを描画
    DrawDirectory(rootPath, registry, context);

    ImGui::End();
}

void ProjectWindow::DrawDirectory(
    const std::filesystem::path& path,
    entt::registry& registry,
    EditorContext& context)
{
    // フォルダの場合
    if (std::filesystem::is_directory(path))
    {
        // フォルダ名をツリーノードとして表示
        bool open = ImGui::TreeNode(path.filename().string().c_str());
        if (open)
        {
            // フォルダ内のファイルを再帰的に表示
            for (const auto& entry : std::filesystem::directory_iterator(path))
            {
                DrawDirectory(entry.path(), registry, context);
            }
            ImGui::TreePop();
        }
    }
    // ファイルの場合
    else if (std::filesystem::is_regular_file(path))
    {
        // モデルファイルのみ表示
        if (!IsModelFile(path)) return;

        std::string filename = path.filename().string();

        // クリックで選択
        if (ImGui::Selectable(filename.c_str()))
        {
            // Entity を作成
            entt::entity e = registry.create();

            // Transform を追加
            auto& t = registry.emplace<TransformComponent>(e);
            t.position = { 0.0f, 0.0f, 0.0f };
            t.scale = { 1.0f, 1.0f, 1.0f };

            // Mesh を追加（パスを相対パスに変換して設定）
            std::string modelPath = path.string();
            // バックスラッシュをスラッシュに統一
            std::replace(modelPath.begin(), modelPath.end(), '\\', '/');
            registry.emplace<MeshComponent>(e, modelPath);

            // Material を追加
            registry.emplace<MaterialComponent>(e).AddPass("pbr");

            // Hierarchy に名前登録
            context.SetName(e, path.stem().string());
            context.Select(e);
        }
    }
}

bool ProjectWindow::IsModelFile(const std::filesystem::path& path)
{
    std::string ext = path.extension().string();

    // 小文字に変換して比較
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    return ext == ".gltf" || ext == ".glb";
}