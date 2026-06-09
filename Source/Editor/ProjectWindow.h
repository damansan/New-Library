#pragma once
#include <string>
#include <vector>
#include <filesystem>

#include "EditorContext.h"
#include "entt.hpp"

class ProjectWindow
{
public:
    // 初期化（検索するルートパスを設定）
    void Initialize(const std::string& rootPath);

    // 描画
    void Draw(entt::registry& registry, EditorContext& context);

private:
    // フォルダツリーを再帰的に描画
    void DrawDirectory(
        const std::filesystem::path& path,
        entt::registry& registry,
        EditorContext& context);

    // ファイルがモデルファイルか判定
    bool IsModelFile(const std::filesystem::path& path);

    // ルートパス
    std::filesystem::path rootPath;
};