#include "AdaptiveTriggerToolWindow.h"
#include <imgui.h>
#include <algorithm>

void AdaptiveTriggerToolWindow::Draw(ControllerComponent& ctrl)
{
    if (!ImGui::Begin("Adaptive Trigger Tool"))
    {
        ImGui::End();
        return;
    }

    DrawTriggerCurve("L2", ctrl.leftTriggerEffect, ctrl.leftTrigger, leftDraggingIndex);
    ImGui::Spacing();
    DrawTriggerCurve("R2", ctrl.rightTriggerEffect, ctrl.rightTrigger, rightDraggingIndex);

    ImGui::End();
}

void AdaptiveTriggerToolWindow::DrawTriggerCurve(const char* label, AdaptiveTrigger::TriggerEffect& effect, float currentValue, int& draggingIndex)
{
    ImGui::PushID(label);
    ImGui::SeparatorText(label);

    auto& points = effect.forcePoints;

    ImVec2 graphSize(ImGui::GetContentRegionAvail().x, 150.0f);
    ImVec2 graphPos = ImGui::GetCursorScreenPos();

    // グラフ全体で1つだけInvisibleButtonを作る
    ImGui::InvisibleButton("##graphBg", graphSize);
    bool bgClicked = ImGui::IsItemClicked(); // クリックした瞬間だけtrue
    bool bgActive = ImGui::IsItemActive();   // 押している間ずっとtrue
    ImVec2 mousePos = ImGui::GetMousePos();

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddRect(graphPos, ImVec2(graphPos.x + graphSize.x, graphPos.y + graphSize.y), IM_COL32(150, 150, 150, 255));

    // position(0~1)/value(0~1) ⇔ 画面座標 の変換
    auto ToScreen = [&](const AdaptiveTrigger::CurvePoint& p) -> ImVec2
        {
            return ImVec2(graphPos.x + p.position * graphSize.x, graphPos.y + (1.0f - p.value) * graphSize.y);
        };
    auto FromScreen = [&](ImVec2 screen) -> AdaptiveTrigger::CurvePoint
        {
            AdaptiveTrigger::CurvePoint p;
            p.position = std::clamp((screen.x - graphPos.x) / graphSize.x, 0.0f, 1.0f);
            p.value = std::clamp(1.0f - (screen.y - graphPos.y) / graphSize.y, 0.0f, 1.0f);
            return p;
        };

    // ポイント同士を線で繋ぐ
    for (size_t i = 0; i + 1 < points.size(); ++i)
    {
        drawList->AddLine(ToScreen(points[i]), ToScreen(points[i + 1]), IM_COL32(217, 90, 48, 255), 2.0f);
    }
    // ポイントを丸で描画（drawListだけなのでImGuiのカーソルには影響しない）
    for (size_t i = 0; i < points.size(); ++i)
    {
        drawList->AddCircleFilled(ToScreen(points[i]), 5.0f, IM_COL32(217, 90, 48, 255));
    }

    const float pickRadius = 8.0f;

    // クリックした瞬間：既存のポイントの近くかどうかを自分で判定する
    if (bgClicked)
    {
        draggingIndex = -1;
        for (size_t i = 0; i < points.size(); ++i)
        {
            ImVec2 screenPos = ToScreen(points[i]);
            float dx = mousePos.x - screenPos.x;
            float dy = mousePos.y - screenPos.y;
            if ((dx * dx + dy * dy) <= (pickRadius * pickRadius))
            {
                draggingIndex = static_cast<int>(i);
                break;
            }
        }

        // 近くにポイントが無ければ、新しいポイントを追加する
        if (draggingIndex == -1)
        {
            points.push_back(FromScreen(mousePos));
            draggingIndex = static_cast<int>(points.size()) - 1;
        }
    }

    // ドラッグ中：マウスを押している間、対象のポイントを追従させる
    if (bgActive && draggingIndex >= 0 && draggingIndex < static_cast<int>(points.size()))
    {
        points[draggingIndex] = FromScreen(mousePos);
    }

    // マウスを離したらドラッグ終了
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
    {
        draggingIndex = -1;
    }

    // ドラッグ中でない時だけ並び替える（ドラッグ中に並びが変わるとindexがズレるため）
    if (draggingIndex == -1)
    {
        std::sort(points.begin(), points.end(),
            [](const AdaptiveTrigger::CurvePoint& a, const AdaptiveTrigger::CurvePoint& b)
            {
                return a.position < b.position;
            });
    }

    float force = AdaptiveTrigger::ComputeForce(effect, currentValue);
    ImGui::Text(u8"現在地: %.3f", currentValue);
    ImGui::Text(u8"出力中の力: %.3f (送信値: %d)", force, static_cast<int>(force * 255.0f));

    auto zones = AdaptiveTrigger::DetectZones(points);
    for (size_t i = 0; i < zones.size(); ++i)
    {
        auto type = AdaptiveTrigger::ClassifyZone(zones[i]);
        const char* typeName =
            type == AdaptiveTrigger::ResistanceType::Constant ? u8"一定抵抗" :
            type == AdaptiveTrigger::ResistanceType::Changing ? u8"変化抵抗" : u8"複雑";

        float zoneStart = zones[i].points.front().position;
        float zoneEnd = zones[i].points.back().position;
        bool isCurrentZone = (currentValue >= zoneStart && currentValue <= zoneEnd);

        ImGui::Text(u8"ゾーン%zu: %s (%.2f~%.2f) %s",
            i + 1, typeName, zoneStart, zoneEnd,
            isCurrentZone ? u8"<- 現在地" : u8"");
    }
    ImGui::PopID();
}