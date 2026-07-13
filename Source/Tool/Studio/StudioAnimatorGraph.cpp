#include "StudioAnimatorGraph.h"
#include "ECS/Components/SkinnedMeshComponent.h"
#include <imgui.h>
#include <fstream>
#include <json.hpp>

void StudioAnimatorGraph::Initialize()
{
    canvas.Initialize();
}

void StudioAnimatorGraph::DrawGUI(
    entt::registry& registry,
    StudioContext& context)
{
    if (!context.showAnimatorGraph) return;

    RefreshAvailableAnimations(registry);

    std::string panelName = "AnimatorGraph##" + std::to_string(windowId);
    ImGui::Begin(panelName.c_str());

    if (!context.attachedGraphPath.empty())
    {
        if (ImGui::Button("Save"))
        {
            Save(context);
        }
        ImGui::SameLine();
        ImGui::TextDisabled("%s", context.attachedGraphPath.c_str());
    }
    else
    {
        ImGui::TextDisabled("No graph attached.");
    }
    ImGui::Separator();

    canvas.Begin("AnimatorCanvas");

    canvas.End();

    ImGui::End();
}

// 現在読み込まれているモデルのアニメーション一覧を取得
void StudioAnimatorGraph::RefreshAvailableAnimations(entt::registry& registry)
{
    availableAnimations.clear();

    auto view = registry.view<SkinnedMeshComponent>();
    if (view.begin() == view.end()) return;

    auto entity = *view.begin();
    auto& skinned = view.get<SkinnedMeshComponent>(entity);

    if (!skinned.model) return;

    for (const auto& anim : skinned.model->GetAnimations())
    {
        availableAnimations.push_back(anim.name);
    }
}

// 保存
void StudioAnimatorGraph::Save(StudioContext& context)
{
    if (context.attachedGraphPath.empty()) return;

    nlohmann::ordered_json j;
    j["sourceModelPath"] = context.currentModelPath;
    j["states"] = nlohmann::json::array();
    j["stateLinks"] = nlohmann::json::array();

    std::ofstream ofs(context.attachedGraphPath);
    if (ofs.is_open())
    {
        ofs << j.dump(4);
    }
}
