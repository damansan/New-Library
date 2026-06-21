#include "AdaptiveTriggerToolWindow.h"
#include <imgui.h>
#include <algorithm>
#include <cmath>

void AdaptiveTriggerToolWindow::Draw(ControllerComponent& ctrl)
{
    if (!ImGui::Begin("Adaptive Trigger Tool"))
    {
        ImGui::End();
        return;
    }

    DrawTriggerSection("L2", ctrl.leftTriggerEffect, leftZone, leftRangeDrag);
    ImGui::Spacing();
    DrawTriggerSection("R2", ctrl.rightTriggerEffect, rightZone, rightRangeDrag);

    ImGui::End();
}

void AdaptiveTriggerToolWindow::DrawTriggerSection(const char* label, AdaptiveTrigger::TriggerEffect& effect, ZoneState& zone, RangeDragState& rangeDrag)
{
    ImGui::PushID(label);

    ImGui::Text(label);

    if (!zone.exists)
    {
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - 80.0f + ImGui::GetCursorPosX());
        if (ImGui::Button(u8"+ ゾーン追加"))
        {
            zone.exists = true;
            zone.start = 0.0f;
            zone.end = 1.0f;
            zone.force = 0.5f;
        }
    }
    else
    {
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - 60.0f + ImGui::GetCursorPosX());
        if (ImGui::Button(u8"削除"))
        {
            zone.exists = false;
        }
    }

    float width = ImGui::GetContentRegionAvail().x;
    DrawRangeBar(zone, width, rangeDrag);

    if (zone.exists)
    {
        ImGui::SliderFloat(u8"強さ", &zone.force, 0.0f, 1.0f);
    }

    // ゾーンの状態を、出力用のforcePointsへ一方的に書き込む（同期はしない）
    auto& points = effect.forcePoints;
    points.clear();
    if (zone.exists)
    {
        const float fadeWidth = 0.02f; // 境界の幅（広げてブレに強くする）

        if (zone.start > 0.0f)
        {
            points.push_back({ std::max(0.0f, zone.start - fadeWidth), 0.0f });
        }
        points.push_back({ zone.start, zone.force });
        points.push_back({ zone.end, zone.force });
        if (zone.end < 1.0f)
        {
            points.push_back({ std::min(1.0f, zone.end + fadeWidth), 0.0f });
        }
    }

    ImGui::PopID();
}

void AdaptiveTriggerToolWindow::DrawRangeBar(ZoneState& zone, float width, RangeDragState& rangeDrag)
{
    ImVec2 pos = ImGui::GetCursorScreenPos();
    const float height = 24.0f;
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    drawList->AddRectFilled(pos, ImVec2(pos.x + width, pos.y + height), IM_COL32(60, 60, 60, 255), 4.0f);

    ImGui::InvisibleButton("##rangeBar", ImVec2(width, height));
    bool barClicked = ImGui::IsItemClicked();
    bool barActive = ImGui::IsItemActive();
    ImVec2 mousePos = ImGui::GetMousePos();

    if (zone.exists)
    {
        float x1 = pos.x + zone.start * width;
        float x2 = pos.x + zone.end * width;

        drawList->AddRectFilled(ImVec2(x1, pos.y), ImVec2(x2, pos.y + height), IM_COL32(217, 90, 48, 255));

        const float handleHalf = 3.0f;
        drawList->AddRectFilled(ImVec2(x1 - handleHalf, pos.y - 3.0f), ImVec2(x1 + handleHalf, pos.y + height + 3.0f), IM_COL32(255, 255, 255, 255));
        drawList->AddRectFilled(ImVec2(x2 - handleHalf, pos.y - 3.0f), ImVec2(x2 + handleHalf, pos.y + height + 3.0f), IM_COL32(255, 255, 255, 255));

        const float pickRange = 8.0f;

        if (barClicked)
        {
            if (std::abs(mousePos.x - x1) <= pickRange)
            {
                rangeDrag.mode = RangeDragState::Mode::Left;
            }
            else if (std::abs(mousePos.x - x2) <= pickRange)
            {
                rangeDrag.mode = RangeDragState::Mode::Right;
            }
            else if (mousePos.x >= x1 && mousePos.x <= x2)
            {
                rangeDrag.mode = RangeDragState::Mode::Move;
                rangeDrag.startMouseX = mousePos.x;
                rangeDrag.startZoneStart = zone.start;
                rangeDrag.startZoneEnd = zone.end;
            }
            else
            {
                rangeDrag.mode = RangeDragState::Mode::None;
            }
        }

        if (barActive)
        {
            float mouseNormX = std::clamp((mousePos.x - pos.x) / width, 0.0f, 1.0f);

            if (rangeDrag.mode == RangeDragState::Mode::Left)
            {
                zone.start = std::clamp(mouseNormX, 0.0f, zone.end - 0.01f);
            }
            else if (rangeDrag.mode == RangeDragState::Mode::Right)
            {
                zone.end = std::clamp(mouseNormX, zone.start + 0.01f, 1.0f);
            }
            else if (rangeDrag.mode == RangeDragState::Mode::Move)
            {
                float rawDelta = (mousePos.x - rangeDrag.startMouseX) / width;
                float zoneWidth = rangeDrag.startZoneEnd - rangeDrag.startZoneStart;
                float clampedDelta = std::clamp(rawDelta, -rangeDrag.startZoneStart, 1.0f - rangeDrag.startZoneEnd);
                zone.start = rangeDrag.startZoneStart + clampedDelta;
                zone.end = rangeDrag.startZoneStart + clampedDelta + zoneWidth;
            }
        }

        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            rangeDrag.mode = RangeDragState::Mode::None;
        }
    }

    ImGui::Dummy(ImVec2(width, height));

    ImGui::TextDisabled("0");
    ImGui::SameLine(width - 24.0f);
    ImGui::TextDisabled("255");
}