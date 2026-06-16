#pragma once
#include <string>
#include <d3d11.h>

#include "IStudioAttachment.h"

#include "Resource/ResourceManager.h"

#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/MeshComponent.h"
#include "ECS/Components/SkinnedMeshComponent.h"
#include "ECS/Components/StaticMeshComponent.h"
#include "ECS/Components/Tags.h"

class StudioModelViewer : public IStudioAttachment
{
public:
    StudioModelViewer() = default;
    ~StudioModelViewer() = default;

    // 初期化
    void Initialize(ID3D11Device* device, ResourceManager* resourceManager);

    // 更新
    void Update(float elapsedTime, StudioContext& context) override;

    // 3D描画
    void OnRender(
        ID3D11DeviceContext* dc,
        const DirectX::XMFLOAT4X4& view,
        const DirectX::XMFLOAT4X4& projection,
        entt::registry& registry,
        StudioContext& context) override 
    {}

    // ImGui
    void DrawGUI(
        entt::registry& registry,
        StudioContext& context) override;

    // モデルをロードしてregistryにEntityを作る
    void LoadModel(entt::registry& registry, const std::string& path);

private:
    // ファイルダイアログを開いてパスを返す
    std::string OpenFileDialog();


    // 現在ロード中のモデルパス
    std::string currentModelPath;

    // ロード済みモデルのEntity
    entt::entity modelEntity = entt::null;

    // デバイスとリソースマネージャ
    ID3D11Device* device = nullptr;
    ResourceManager* resourceManager = nullptr;
};