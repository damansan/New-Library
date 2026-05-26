#include "MeshLoaderSystem.h"

void MeshLoaderSystem::Update(
    entt::registry& registry,
    ID3D11Device* device,
    ResourceManager& resourceManager)
{
    // MeshComponentを持つがMeshLoadedTagのないEntityを取得
    auto view = registry.view<MeshComponent>(entt::exclude<MeshLoadedTag>);

    for (auto entity : view)
    {
        auto& mesh = view.get<MeshComponent>(entity);

        // モデルパスが空ならスキップ
        if (mesh.modelPath.empty()) 
        {
            continue;
        }

        // モデル読み込み（キャッシュ利用）
        auto model = resourceManager.GetModel(device, mesh.modelPath);
        if (!model) continue;

        // スキニング有無で自動判定
        if (model->HasSkin())
        {
            // スキニングあり (SkinnedMeshComponent + AnimatorComponent付与)
            auto& skinned = registry.emplace<SkinnedMeshComponent>(entity);
            skinned.model = model;

            // ボーン行列初期化
            skinned.boneTransforms.resize(
                model->GetNodes().size(),
                { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 }
            );

            // アニメーション初期設定
            auto& animator = registry.emplace<AnimatorComponent>(entity);
            if (!model->GetAnimations().empty())
            {
                animator.currentAnimation =
                    model->GetAnimations()[0].name;
            }
        }
        else
        {
            // スキニングなし → StaticMeshComponent付与
            auto& staticMesh = registry.emplace<StaticMeshComponent>(entity);
            staticMesh.model = model;
        }

        // ロード済みフラグを付与
        registry.emplace<MeshLoadedTag>(entity);
    }
}