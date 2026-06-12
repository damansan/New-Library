#include "ControllerToolWindow.h"
#include <imgui.h>
#include <cmath>

// 描画
void ControllerToolWindow::Draw(entt::registry& registry, ControllerSystem& system)
{
    ImGui::Begin("Controller Tool");

    // ControllerComponentを持つEntit を探す
    auto view = registry.view<ControllerComponent>();
    if (view.empty())
    {
        ImGui::TextColored({ 1.0f, 0.4f, 0.4f, 1.0f }, u8"ControllerComponent を持つ Entity がありません");
        ImGui::End();
        return;
    }

    // 先頭の Entity を使う
    auto entity = *view.begin();
    auto& ctrl = view.get<ControllerComponent>(entity);
    auto& settings = system.GetSettings();

    // タブバー
    if (ImGui::BeginTabBar("ControllerTabs"))
    {
        if (ImGui::BeginTabItem("Monitor"))
        {
            DrawMonitorTab(ctrl);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Stick"))
        {
            DrawStickTab(settings, ctrl);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Trigger"))
        {
            DrawTriggerTab(settings, ctrl);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Adaptive"))
        {
            DrawAdaptiveTriggerTab(settings, ctrl);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Vibration"))
        {
            DrawVibrationTab(settings, ctrl);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::End();
}

// モニタータブ
void ControllerToolWindow::DrawMonitorTab(const ControllerComponent& ctrl)
{
    // 接続状態
    if (ctrl.connected)
    {
        const char* typeName = ctrl.type == ControllerType::PS5 ? "PS5 DualSense" : "Xbox";
        ImGui::TextColored({ 0.3f, 1.0f, 0.3f, 1.0f }, u8"[%s] 接続中", typeName);
    }
    else
    {
        ImGui::TextColored({ 0.6f, 0.6f, 0.6f, 1.0f }, u8"未接続");
        return;
    }

    ImGui::Separator();

    // スティック
    ImGui::Text("Left  Stick  X: %.3f  Y: %.3f", ctrl.leftStickX, ctrl.leftStickY);
    ImGui::Text("Right Stick  X: %.3f  Y: %.3f", ctrl.rightStickX, ctrl.rightStickY);

    ImGui::Separator();

    // トリガー
    ImGui::Text("L2"); ImGui::SameLine();
    ImGui::ProgressBar(ctrl.leftTrigger, { 150, 0 }, "L2");
    ImGui::Text("R2"); ImGui::SameLine();
    ImGui::ProgressBar(ctrl.rightTrigger, { 150, 0 }, "R2");

    ImGui::Separator();

    // ボタン
    auto showBtn = [&](const char* label, uint32_t mask)
        {
            bool pressed = ctrl.IsPressed(mask);
            if (pressed) ImGui::PushStyleColor(ImGuiCol_Button, { 0.2f, 0.8f, 0.2f, 1.0f });
            ImGui::SmallButton(label);
            if (pressed) ImGui::PopStyleColor();
            ImGui::SameLine();
        };

    showBtn(u8"×", ControllerButtons::Cross);
    showBtn(u8"○", ControllerButtons::Circle);
    showBtn(u8"□", ControllerButtons::Square);
    showBtn(u8"△", ControllerButtons::Triangle);
    showBtn("L1", ControllerButtons::L1);
    showBtn("R1", ControllerButtons::R1);
    showBtn("L2", ControllerButtons::L2);
    showBtn("R2", ControllerButtons::R2);
    showBtn("L3", ControllerButtons::L3);
    showBtn("R3", ControllerButtons::R3);
    showBtn("OPT", ControllerButtons::Options);
    showBtn("SHR", ControllerButtons::Share);
    ImGui::NewLine();
}

// スティック設定タブ
void ControllerToolWindow::DrawStickTab(ControllerSettings& settings, const ControllerComponent& ctrl)
{
    // スティックビジュアルを描画するヘルパー
    auto drawStickVisual = [](
        const char* label,
        float rawX, float rawY,
        float deadzone)
        {
            const float radius = 60.0f;  // 外円の半径
            const float dotRadius = 5.0f;  // スティック位置の点の半径

            ImGui::Text("%s", label);

            // 描画の基準座標（円の中心）
            ImVec2 canvasPos = ImGui::GetCursorScreenPos();
            ImVec2 center = { canvasPos.x + radius, canvasPos.y + radius };

            ImDrawList* dl = ImGui::GetWindowDrawList();

            // 外側の円（白塗り背景 + 黒枠）
            dl->AddCircleFilled(center, radius, IM_COL32(40, 40, 40, 255));
            dl->AddCircle(center, radius, IM_COL32(200, 200, 200, 255), 64, 2.0f);

            // 内側の赤円（デッドゾーン）
            float dzRadius = radius * deadzone;
            dl->AddCircle(center, dzRadius, IM_COL32(220, 50, 50, 255), 64, 1.5f);

            // 十字線（中心のガイド）
            dl->AddLine({ center.x - radius, center.y }, { center.x + radius, center.y },
                IM_COL32(80, 80, 80, 255), 1.0f);
            dl->AddLine({ center.x, center.y - radius }, { center.x, center.y + radius },
                IM_COL32(80, 80, 80, 255), 1.0f);

            // スティック位置の点
            ImVec2 dotPos =
            {
                center.x + rawX * radius,
                center.y - rawY * radius   // Y軸は画面下がプラスなので反転
            };
            dl->AddCircleFilled(dotPos, dotRadius, IM_COL32(255, 255, 255, 255));

            // デッドゾーン内なら点を赤くする
            float dist = sqrtf(rawX * rawX + rawY * rawY);
            if (dist < deadzone)
            {
                dl->AddCircleFilled(dotPos, dotRadius, IM_COL32(220, 50, 50, 255));
            }

            // ImGui のカーソルを円の下に進める
            ImGui::Dummy({ radius * 2.0f, radius * 2.0f });
        };

    // デッドゾーン適用後の計算
    auto applyDZ = [](float v, float dz) -> float
        {
            if (fabsf(v) < dz) return 0.0f;
            float sign = v > 0.0f ? 1.0f : -1.0f;
            return sign * (fabsf(v) - dz) / (1.0f - dz);
        };

    // --- Left Stick ---
    ImGui::SeparatorText(u8"Left Stick");
    drawStickVisual("L", ctrl.leftStickX, ctrl.leftStickY, settings.leftStickDeadzone);

    ImGui::SliderFloat(u8"デッドゾーン##LS", &settings.leftStickDeadzone, 0.0f, 1.0f, "%.2f");

    float lx = applyDZ(ctrl.leftStickX, settings.leftStickDeadzone);
    float ly = applyDZ(ctrl.leftStickY, settings.leftStickDeadzone);
    ImGui::Text(u8"Raw  X: %.3f  Y: %.3f", ctrl.leftStickX, ctrl.leftStickY);
    ImGui::Text(u8"Post X: %.3f  Y: %.3f", lx, ly);

    ImGui::Spacing();

    // --- Right Stick ---
    ImGui::SeparatorText(u8"Right Stick");
    drawStickVisual("R", ctrl.rightStickX, ctrl.rightStickY, settings.rightStickDeadzone);

    ImGui::SliderFloat(u8"デッドゾーン##RS", &settings.rightStickDeadzone, 0.0f, 1.0f, "%.2f");

    float rx = applyDZ(ctrl.rightStickX, settings.rightStickDeadzone);
    float ry = applyDZ(ctrl.rightStickY, settings.rightStickDeadzone);
    ImGui::Text(u8"Raw  X: %.3f  Y: %.3f", ctrl.rightStickX, ctrl.rightStickY);
    ImGui::Text(u8"Post X: %.3f  Y: %.3f", rx, ry);
}

// トリガー設定タブ
void ControllerToolWindow::DrawTriggerTab(ControllerSettings& settings, const ControllerComponent& ctrl)
{
    ImGui::SeparatorText(u8"L2 トリガー");

    ImGui::SliderFloat(u8"デッドゾーン##LT", &settings.leftTriggerDeadzone, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat(u8"判定閾値##LT", &settings.leftTriggerThreshold, 0.0f, 1.0f, "%.2f");

    // 生の値
    ImGui::Text(u8"Raw: %.3f", ctrl.leftTrigger);

    // デッドゾーン適用後プレビュー
    auto applyDZ = [](float v, float dz) -> float
        {
            if (v < dz) return 0.0f;
            return (v - dz) / (1.0f - dz);
        };

    float lt = applyDZ(ctrl.leftTrigger, settings.leftTriggerDeadzone);
    ImGui::Text(u8"Post: %.3f", lt);

    // 判定閾値の視覚表示
    ImGui::ProgressBar(ctrl.leftTrigger, { -1, 0 }, "");
    // 閾値ラインをProgressBarの上に描画
    {
        ImVec2 barMin = ImGui::GetItemRectMin();
        ImVec2 barMax = ImGui::GetItemRectMax();
        float  x = barMin.x + (barMax.x - barMin.x) * settings.leftTriggerThreshold;
        ImGui::GetWindowDrawList()->AddLine(
            { x, barMin.y }, { x, barMax.y },
            IM_COL32(255, 80, 80, 255), 2.0f
        );
    }
    ImGui::Text(u8"  ↑ 赤線 = 判定閾値");

    // デジタル判定結果
    bool l2 = ctrl.leftTrigger >= settings.leftTriggerThreshold;
    ImGui::TextColored(
        l2 ? ImVec4(0.2f, 1.0f, 0.2f, 1.0f) : ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
        u8"L2 判定: %s", l2 ? "ON" : "OFF"
    );

    ImGui::Spacing();
    ImGui::SeparatorText(u8"R2 トリガー");

    ImGui::SliderFloat(u8"デッドゾーン##RT", &settings.rightTriggerDeadzone, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat(u8"判定閾値##RT", &settings.rightTriggerThreshold, 0.0f, 1.0f, "%.2f");

    ImGui::Text(u8"Raw: %.3f", ctrl.rightTrigger);

    float rt = applyDZ(ctrl.rightTrigger, settings.rightTriggerDeadzone);
    ImGui::Text(u8"Post: %.3f", rt);

    ImGui::ProgressBar(ctrl.rightTrigger, { -1, 0 }, "");
    {
        ImVec2 barMin = ImGui::GetItemRectMin();
        ImVec2 barMax = ImGui::GetItemRectMax();
        float  x = barMin.x + (barMax.x - barMin.x) * settings.rightTriggerThreshold;
        ImGui::GetWindowDrawList()->AddLine(
            { x, barMin.y }, { x, barMax.y },
            IM_COL32(255, 80, 80, 255), 2.0f
        );
    }
    ImGui::Text(u8"  ↑ 赤線 = 判定閾値");

    bool r2 = ctrl.rightTrigger >= settings.rightTriggerThreshold;
    ImGui::TextColored(
        r2 ? ImVec4(0.2f, 1.0f, 0.2f, 1.0f) : ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
        u8"R2 判定: %s", r2 ? "ON" : "OFF"
    );
}

// アダプティブトリガータブ
void ControllerToolWindow::DrawAdaptiveTriggerTab(ControllerSettings& settings, const ControllerComponent& ctrl)
{
    if (ctrl.type != ControllerType::PS5)
    {
        ImGui::TextColored({ 0.6f, 0.6f, 0.6f, 1.0f }, u8"PS5 接続時のみ使用できます");
        return;
    }

    const char* effectNames[] = { u8"Off（無効）", u8"Rigid（一定抵抗）", u8"Pulse（振動）", u8"Slope（傾斜）" };

    auto drawTriggerEffect = [&](const char* label, TriggerEffect& effect)
        {
            ImGui::SeparatorText(label);

            int type = (int)effect.type;
            if (ImGui::Combo(u8"エフェクト", &type, effectNames, 4))
            {
                effect.type = (TriggerEffectType)type;
            }

            // Off以外のときだけパラメータを表示
            if (effect.type != TriggerEffectType::Off)
            {
                int start = effect.start;
                int end = effect.end;
                int force = effect.force;

                if (ImGui::SliderInt(u8"開始位置##start", &start, 0, 255)) effect.start = (uint8_t)start;
                if (ImGui::SliderInt(u8"終了位置##end", &end, 0, 255)) effect.end = (uint8_t)end;
                if (ImGui::SliderInt(u8"抵抗の強さ##force", &force, 0, 255)) effect.force = (uint8_t)force;

                // Pulse のときだけ周波数を表示
                if (effect.type == TriggerEffectType::Pulse)
                {
                    int freq = effect.freq;
                    if (ImGui::SliderInt(u8"振動周波数##freq", &freq, 0, 255)) effect.freq = (uint8_t)freq;
                }
            }
        };

    drawTriggerEffect("L2", settings.leftTriggerEffect);
    ImGui::Spacing();
    drawTriggerEffect("R2", settings.rightTriggerEffect);
}

// 振動タブ
void ControllerToolWindow::DrawVibrationTab(ControllerSettings& settings, ControllerComponent& ctrl)
{
    ImGui::SeparatorText(u8"振動テスト");

    int left = (int)settings.leftMotor;
    int right = (int)settings.rightMotor;

    if (ImGui::SliderInt(u8"左モーター##vib", &left, 0, 255)) settings.leftMotor = (uint8_t)left;
    if (ImGui::SliderInt(u8"右モーター##vib", &right, 0, 255)) settings.rightMotor = (uint8_t)right;

    // スライダーの値を ControllerComponent に反映（ControllerSystem が毎フレーム送信）
    ctrl.vibration.leftMotor = settings.leftMotor;
    ctrl.vibration.rightMotor = settings.rightMotor;

    ImGui::Spacing();

    // 一括停止ボタン
    if (ImGui::Button(u8"停止"))
    {
        settings.leftMotor = 0;
        settings.rightMotor = 0;
        ctrl.vibration.leftMotor = 0;
        ctrl.vibration.rightMotor = 0;
    }

    if (ctrl.type != ControllerType::PS5)
    {
        ImGui::Spacing();
        ImGui::TextColored({ 0.6f, 0.6f, 0.6f, 1.0f }, u8"※ アダプティブトリガーは PS5 接続時のみ有効");
    }
}
