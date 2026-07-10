#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <d3d11.h>
#include <wrl.h>

#include "EditorContext.h"
#include "entt.hpp"

// フォルダ・ファイルの操作状態
enum class ProjectItemMode
{
    None,            // 何もしていない
    Renaming,        // 名前編集中
    ConfirmingDelete,// 削除確認ダイアログ表示中
};

// フォルダ・ファイルの操作状態まとめ
struct ProjectItemState
{
    std::filesystem::path selectedPath;   // 選択中のファイル・フォルダ
    ProjectItemMode        mode = ProjectItemMode::None;
    std::filesystem::path  targetPath;    // リネーム・削除対象のパス
    char                   editBuffer[256] = {}; // リネーム中の入力バッファ
};


class ProjectWindow
{
public:
    // 初期化（走査するルートパスを設定）
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
    // ファイルがアニメーショングラフファイルか判定
    bool IsAnimGraphFile(const std::filesystem::path& path);

    // 重複しない名前で作成
    std::string GenerateUniqueName(const std::filesystem::path& parentDir, const std::string& baseName);

    // 削除確認ポップ
    void DrawDeleteConfirmPopup();

    // リネーム開始
    void BeginRename(const std::filesystem::path& target);
    // リネーム確定
    void CommitRename();

    // ルートパス
    std::filesystem::path rootPath;

    // フォルダ・ファイル共通の操作状態
    ProjectItemState itemState;

    // モデルアイコン用SRV
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> modelIconSRV;


};