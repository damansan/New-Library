#include "StudioAnimatorGraph.h"
#include <imgui.h>

void StudioAnimatorGraph::Initialize()
{
    canvas.Initialize();
}

void StudioAnimatorGraph::DrawGUI(
    entt::registry& registry,
    StudioContext& context)
{
    if (!context.showAnimatorGraph) return;

    std::string panelName = "AnimatorGraph##" + std::to_string(windowId);
    ImGui::Begin(panelName.c_str());

    canvas.Begin("AnimatorCanvas");

    canvas.End();

    ImGui::End();
}
