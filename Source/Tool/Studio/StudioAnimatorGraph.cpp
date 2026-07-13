#include "StudioAnimatorGraph.h"
#include "ECS/Components/SkinnedMeshComponent.h"
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

    if (!loaded && !context.attachedGraphPath.empty())
    {
        Load(context);
        loaded = true;
    }


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

    // 既存のState描画
    for (auto& state : graph.states)
    {
        ed::NodeId nodeId(state.id);
        ed::BeginNode(nodeId);
        ImGui::Text("%s", state.name.c_str());
        ed::EndNode();
    }

    // 右クリックで新規State作成メニュー
    ed::Suspend();
    if (ed::ShowBackgroundContextMenu())
    {
        pendingCreatePos = ImGui::GetMousePos();
        ImGui::OpenPopup("CreateNewState");
    }
    if (ImGui::BeginPopup("CreateNewState"))
    {
        if (ImGui::MenuItem(u8"State"))
        {
            ImVec2 canvasPos = ed::ScreenToCanvas(pendingCreatePos);
            int newId = graph.CreateState("New State", canvasPos.x, canvasPos.y);
            ed::SetNodePosition(ed::NodeId(newId), canvasPos);
        }
        ImGui::EndPopup();
    }
    ed::Resume();

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

    nlohmann::ordered_json graphJson = graph.ToJson();
    j["states"] = graphJson["states"];
    j["stateLinks"] = graphJson["stateLinks"];

    std::ofstream ofs(context.attachedGraphPath);
    if (ofs.is_open())
    {
        ofs << j.dump(4);
    }
}

// 読み込み
void StudioAnimatorGraph::Load(StudioContext& context)
{
    std::ifstream ifs(context.attachedGraphPath);
    if (!ifs.is_open()) return;

    try
    {
        nlohmann::json j;
        ifs >> j;
        graph.FromJson(j);
    }
    catch (const std::exception&)
    {
        // 壊れているか空のjsonの場合なにもしない
    }
}
