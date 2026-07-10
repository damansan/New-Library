#pragma once
#include <string>
#include <unordered_map>
#include "entt.hpp"

// カメラモード
enum class CameraMode
{
    Debug,  // デバッグカメラ（マウス・キーボード）
    Game,   // ゲームカメラ  （コントローラー）
};

// Studioウィンドウを開くリクエスト
struct StudioOpenRequest
{
    bool        pending = false;
    std::string assetPath;
    std::string graphPath;
};

struct EditorContext
{
    entt::entity selectedEntity = entt::null;
    std::unordered_map<entt::entity, std::string> entityNames;

    // カメラモード
    CameraMode cameraMode = CameraMode::Debug;
    // 右クリックを押したか
    bool rightClickPressed = false;  

    // Studioウィンドウ開くリクエスト
    StudioOpenRequest studioRequest;

    std::string GetName(entt::entity e) const
    {
        auto it = entityNames.find(e);
        if (it != entityNames.end()) return it->second;
        return "Entity " + std::to_string((uint32_t)e);
    }

    void SetName(entt::entity e, const std::string& name)
    {
        entityNames[e] = name;
    }

    void Select(entt::entity e) { selectedEntity = e; }
    void Deselect() { selectedEntity = entt::null; }

    bool IsSelected(entt::entity e) const
    {
        return selectedEntity == e;
    }
};