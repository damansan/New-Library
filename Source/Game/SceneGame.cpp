#include "SceneGame.h"
#include <imgui.h>

void SceneGame::Initialize(const SceneContext& context)
{
    renderSystem.Initialize(context.device);
    debugRenderer.Initialize(context.device);

    // ランバートシェーダーを使うEntity
    auto entity = registry.create();
    auto& t = registry.emplace<TransformComponent>(entity);
    t.position = { -3.0f, 0.0f, 0.0f };
    registry.emplace<MeshComponent>(entity, "Data/Model/Mr.Incredible.glb");
    registry.emplace<MaterialComponent>(entity).AddPass("lambert");  // ランバートを指定

    // ベーシックシェーダーを使うEntity
    auto entity2 = registry.create();
    auto& t2 = registry.emplace<TransformComponent>(entity2);
    t2.position = { 3.0f, 0.0f, 0.0f };
    registry.emplace<MeshComponent>(entity2, "Data/Model/Mr.Incredible.glb");
    registry.emplace<MaterialComponent>(entity2).AddPass("basic");    // ベーシックを指定


    // スキニングモデル
    auto entity3 = registry.create();
    registry.emplace<TransformComponent>(entity3);
    registry.emplace<MeshComponent>(entity3, "Data/Model/unitychan.glb");
    registry.emplace<MaterialComponent>(entity3).AddPass("skinning_lambert");
}

void SceneGame::Update(float elapsedTime, const SceneContext& context)
{
    // 未ロードのモデルを自動ロード
    meshLoaderSystem.Update(
        registry,
        context.device,
        *context.resourceManager
    );
    animationSystem.Update(registry, elapsedTime/5);
}

void SceneGame::Render(const SceneContext& context)
{
    sceneContext = &context;

    // CbScene更新
    CbScene cbSceneData{};
    cbSceneData.view = context.view;
    cbSceneData.projection = context.projection;


    // ライト設定（仮の平行光源）
    cbSceneData.lightDirection = { 0.5f, -1.0f, 0.5f, 0.0f };
    cbSceneData.lightColor = { 1.0f,  1.0f, 1.0f, 1.0f };
    cbSceneData.ambientColor = { 0.2f,  0.2f, 0.2f, 1.0f };

    // カメラ位置
    if (context.camera)
    {
        const auto& eye = context.camera->GetEye();
        cbSceneData.cameraPosition = { eye.x, eye.y, eye.z, 0.0f };
    }

    context.deviceContext->UpdateSubresource(
        renderSystem.GetCbScene(), 0, nullptr, &cbSceneData, 0, 0
    );

    // 頂点シェーダーに設定
    context.deviceContext->VSSetConstantBuffers(
        0, 1, renderSystem.GetCbSceneAddress()
    );

    // ピクセルシェーダーにも設定
    context.deviceContext->PSSetConstantBuffers(
        0, 1, renderSystem.GetCbSceneAddress()
    );

    renderSystem.Render(
        registry,
        context.deviceContext,
        context.renderTargetView,
        context.depthStencilView
    );

    ////////////////////////

    debugRenderer.Clear();

    // SkinnedMeshComponent を持つエンティティのボーンを追加
    auto view = registry.view<SkinnedMeshComponent, TransformComponent>();
    view.each([&](auto entity, SkinnedMeshComponent& skinned, TransformComponent& transform)
        {
            if (!skinned.model) return;

            debugRenderer.AddBones(
                skinned.model->GetNodes(),
                transform.GetWorldMatrix(),
                { 1.0f, 1.0f, 0.0f, 1.0f }  // 黄色
            );
        });

    // ボーン描画
    debugRenderer.Render(
        context.deviceContext,
        context.view,
        context.projection
    );
}

void SceneGame::RenderImGui()
{
    auto view = registry.view<SkinnedMeshComponent, AnimatorComponent>();
    view.each([&](auto entity, SkinnedMeshComponent& skinned, AnimatorComponent& animator)
        {
            if (!skinned.model) return;

            const auto& model = *skinned.model;
            if (model.GetAnimations().empty()) return;

            const auto& anim = model.GetAnimations()[0]; // Combo1
            if (anim.nodeAnims.size() <= 3) return;

            const auto& hipsAnim = anim.nodeAnims[3]; // Hips
            if (hipsAnim.rotationKeyframes.empty()) return;

            ImGui::Begin("Hips Rotation All Keyframes");
            ImGui::Text("Anim: %s  (%.3fsec)",
                anim.name.c_str(), anim.secondsLength);
            ImGui::Text("Hips rotation keyframes: %d",
                (int)hipsAnim.rotationKeyframes.size());
            ImGui::Separator();

            // 全キーフレームを表示
            for (int i = 0; i < (int)hipsAnim.rotationKeyframes.size(); i++)
            {
                const auto& kf = hipsAnim.rotationKeyframes[i];
                ImGui::Text("[%2d] t=%.4f  x=%6.3f y=%6.3f z=%6.3f w=%6.3f",
                    i, kf.seconds,
                    kf.value.x, kf.value.y,
                    kf.value.z, kf.value.w);
            }

            ImGui::Separator();

            // 現在時刻のサンプリング結果も表示
            ImGui::Text("CurrentTime: %.4f", animator.animationTime);
            const auto& nodes = model.GetNodes();
            if (nodes.size() > 3)
            {
                const auto& hips = nodes[3];
                ImGui::Text("Hips node.rotation now:");
                ImGui::Text("  x=%6.3f y=%6.3f z=%6.3f w=%6.3f",
                    hips.rotation.x, hips.rotation.y,
                    hips.rotation.z, hips.rotation.w);
            }

            ImGui::End();
        });
}