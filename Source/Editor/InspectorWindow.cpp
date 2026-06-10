#include "InspectorWindow.h"
#include <imgui.h>
//#include "ComponentRegistry.h"

// 描画
void InspectorWindow::Draw(entt::registry& registry, EditorContext& context)
{
    ImGui::Begin("Inspector");

    // 何も選択されていなければ何も表示しない
    if (context.selectedEntity == entt::null)
    {
        ImGui::Text("No entity selected.");
        ImGui::End();
        return;
    }

    // 名前編集
    DrawNameField(registry, context);

    ImGui::Separator();

    // コンポーネント一覧
    DrawComponents(registry, context);

    ImGui::End();
}

// 名前編集UI
void InspectorWindow::DrawNameField(entt::registry& registry, EditorContext& context)
{
}

// コンポーネント一覧UI
void InspectorWindow::DrawComponents(entt::registry& registry, EditorContext& context)
{
}
