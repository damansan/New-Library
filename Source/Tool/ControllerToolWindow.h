#pragma once
#include <imgui.h>
#include "entt.hpp"
#include "ECS/Components/ControllerComponent.h"
#include "ECS/Systems/ControllerSystem.h"

class ControllerToolWindow
{
public:
    ControllerToolWindow() = default;
    ~ControllerToolWindow() = default;

    ControllerToolWindow(const ControllerToolWindow&) = delete;
    ControllerToolWindow& operator=(const ControllerToolWindow&) = delete;
    ControllerToolWindow(ControllerToolWindow&&) = delete;
    ControllerToolWindow& operator=(ControllerToolWindow&&) = delete;

    // 描画
    void Draw(entt::registry& registry, ControllerSystem& system);

private:
    // モニタータブ（接続状態/生の入力値確認）
    void DrawMonitorTab(const ControllerComponent& ctrl);

    // スティック設定タブ（デッドゾーン調整）
    void DrawStickTab(ControllerSettings& settings, const ControllerComponent& ctrl);

    // トリガー設定タブ（デッドゾーン/判定閾値）
    void DrawTriggerTab(ControllerSettings& settings, const ControllerComponent& ctrl);

    // アダプティブトリガータブ（PS5のみ）
    void DrawAdaptiveTriggerTab(ControllerSettings& settings, const ControllerComponent& ctrl);

    // 振動タブ
    void DrawVibrationTab(ControllerSettings& settings, ControllerComponent& ctrl);
};