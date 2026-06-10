#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <d3d11.h>
#include <wrl.h>

#include "EditorContext.h"
#include "entt.hpp"

class ProjectWindow
{
public:
    // 初期化（検索するルートパスを設定）
    void Initialize(const std::string& rootPath);
    // アイコン読み込み
    void LoadIcons(ID3D11Device* device);
    // 描画
    void Draw(entt::registry& registry, EditorContext& context);

private:
    // 左側：フォルダツリー
    void DrawFolderTree(const std::filesystem::path& path);
    // 右側：ファイル一覧
    void DrawFileGrid(entt::registry& registry, EditorContext& context);
    // ファイルがモデルファイルか判定
    bool IsModelFile(const std::filesystem::path& path);

    // ルートパス
    std::filesystem::path rootPath;
    // 現在選択中のフォルダ
    std::filesystem::path selectedFolder;

    // モデルアイコン用SRV
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> modelIconSRV;
};