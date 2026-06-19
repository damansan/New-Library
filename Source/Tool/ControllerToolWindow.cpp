#include "ControllerToolWindow.h"
#include <imgui.h>
#include <cmath>
#include <algorithm>
#include <DirectXMath.h>


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
			const float radius = 60.0f;    // 外円の半径
			const float dotRadius = 5.0f;  // スティック位置の点の半径

			ImGui::Text("%s", label);

			// 描画の基準座標（円の中心）
			ImVec2 canvasPos = ImGui::GetCursorScreenPos();
			ImVec2 center = { canvasPos.x + radius, canvasPos.y + radius };

			ImDrawList* dl = ImGui::GetWindowDrawList();

			// 倒し切り判定
			float dist = sqrtf(rawX * rawX + rawY * rawY);
			bool isMaxed = dist >= 0.99f; // 少し緩め

			// 外側の円
			dl->AddCircleFilled(center, radius, IM_COL32(40, 40, 40, 255));
			dl->AddCircle(center, radius, isMaxed ? IM_COL32(80, 220, 80, 255) : IM_COL32(200, 200, 200, 255), 64, isMaxed ? 3.0f : 2.0f);

			// 内側の赤円（デッドゾーン）
			float dzRadius = radius * deadzone;
			dl->AddCircle(center, dzRadius, IM_COL32(220, 50, 50, 255), 64, 1.5f);

			// 十字線（中心のガイド）
			dl->AddLine({ center.x - radius, center.y }, { center.x + radius, center.y }, IM_COL32(80, 80, 80, 255), 1.0f);
			dl->AddLine({ center.x, center.y - radius }, { center.x, center.y + radius }, IM_COL32(80, 80, 80, 255), 1.0f);

			// スティック位置の点
			ImVec2 dotPos =
			{
				center.x + rawX * radius,
				center.y - rawY * radius
			};

			ImU32 dotColor = IM_COL32(255, 255, 255, 255);
			if (isMaxed)
			{
				dotColor = IM_COL32(80, 220, 80, 255); // 倒し切り
			}
			else if (dist < deadzone)
			{
				dotColor = IM_COL32(220, 50, 50, 255); // デッドゾーン内
			}
			dl->AddCircleFilled(dotPos, dotRadius, dotColor);

			ImGui::Dummy({ radius * 2.0f, radius * 2.0f });
		};

	// デッドゾーン適用後の計算
	auto applyDZ = [](float v, float dz) -> float
		{
			if (fabsf(v) < dz) return 0.0f;
			float sign = v > 0.0f ? 1.0f : -1.0f;
			return sign * (fabsf(v) - dz) / (1.0f - dz);
		};

	const float panelWidth = 220.0f;

	// --- Left Stick ---
	ImGui::BeginChild("LeftStickPanel", ImVec2(panelWidth, 240.0f), false);
	ImGui::SeparatorText(u8"Left Stick");
	drawStickVisual("L", ctrl.leftStickX, ctrl.leftStickY, settings.leftStickDeadzone);

	ImGui::SetNextItemWidth(150.0f);
	ImGui::DragFloat(u8"デッドゾーン##LS", &settings.leftStickDeadzone, 0.005f, 0.0f, 1.0f, "%.2f");

	float lx = applyDZ(ctrl.leftStickX, settings.leftStickDeadzone);
	float ly = applyDZ(ctrl.leftStickY, settings.leftStickDeadzone);
	ImGui::Text(u8"Raw  X: %.3f  Y: %.3f", ctrl.leftStickX, ctrl.leftStickY);
	ImGui::Text(u8"Post X: %.3f  Y: %.3f", lx, ly);
	ImGui::EndChild();

	ImGui::SameLine();

	// --- Right Stick ---
	ImGui::BeginChild("RightStickPanel", ImVec2(panelWidth, 240.0f), false);
	ImGui::SeparatorText(u8"Right Stick");
	drawStickVisual("R", ctrl.rightStickX, ctrl.rightStickY, settings.rightStickDeadzone);

	ImGui::SetNextItemWidth(150.0f);
	ImGui::DragFloat(u8"デッドゾーン##RS", &settings.rightStickDeadzone, 0.005f, 0.0f, 1.0f, "%.2f");

	float rx = applyDZ(ctrl.rightStickX, settings.rightStickDeadzone);
	float ry = applyDZ(ctrl.rightStickY, settings.rightStickDeadzone);
	ImGui::Text(u8"Raw  X: %.3f  Y: %.3f", ctrl.rightStickX, ctrl.rightStickY);
	ImGui::Text(u8"Post X: %.3f  Y: %.3f", rx, ry);
	ImGui::EndChild();
}

// トリガー設定タブ
void ControllerToolWindow::DrawTriggerTab(ControllerSettings& settings, const ControllerComponent& ctrl)
{
    // 横ゲージ（プログレスバー＋デッドゾーン境界線＋閾値ライン）を描くヘルパー
    auto drawTriggerBar = [](float value, float deadzone, float threshold)
        {
            ImGui::ProgressBar(value, { -1, 0 }, "");

            ImVec2 barMin = ImGui::GetItemRectMin();
            ImVec2 barMax = ImGui::GetItemRectMax();
            float  barWidth = barMax.x - barMin.x;
            ImDrawList* dl = ImGui::GetWindowDrawList();

            // デッドゾーンの境界線（赤）※スティックタブと同色
            float dzX = barMin.x + barWidth * deadzone;
            dl->AddLine({ dzX, barMin.y }, { dzX, barMax.y }, IM_COL32(220, 50, 50, 255), 2.0f);

            // 判定閾値ライン（緑）※スティックタブと同色
            float thX = barMin.x + barWidth * threshold;
            dl->AddLine({ thX, barMin.y }, { thX, barMax.y }, IM_COL32(80, 220, 80, 255), 2.0f);
        };

    // デッドゾーン適用後の計算
    auto applyDZ = [](float v, float dz) -> float
        {
            if (v < dz) return 0.0f;
            return (v - dz) / (1.0f - dz);
        };

    const float panelWidth = 220.0f;

    // --- L2 ---
    ImGui::BeginChild("L2TriggerPanel", ImVec2(panelWidth, 220.0f), false);
    ImGui::SeparatorText(u8"L2 トリガー");

	ImGui::Text(u8"デッドゾーン");
	ImGui::SetNextItemWidth(-1.0f); // ゲージ(ProgressBar)と同じ幅にする
	ImGui::SliderFloat(u8"##LT_DZ", &settings.leftTriggerDeadzone, 0.0f, 1.0f, "%.2f");

	ImGui::Text(u8"判定閾値");
	ImGui::SetNextItemWidth(-1.0f);
	ImGui::SliderFloat(u8"##LT_TH", &settings.leftTriggerThreshold, 0.0f, 1.0f, "%.2f");

    ImGui::Text(u8"Raw: %.3f", ctrl.leftTrigger);

    float lt = applyDZ(ctrl.leftTrigger, settings.leftTriggerDeadzone);
    ImGui::Text(u8"Post: %.3f", lt);

    drawTriggerBar(ctrl.leftTrigger, settings.leftTriggerDeadzone, settings.leftTriggerThreshold);
    ImGui::Text(u8"  赤線 = デッドゾーン ｜ 緑線 = 判定閾値");

    bool l2 = ctrl.leftTrigger >= settings.leftTriggerThreshold;
    ImGui::TextColored(
        l2 ? ImVec4(0.2f, 1.0f, 0.2f, 1.0f) : ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
        u8"L2 判定: %s", l2 ? "ON" : "OFF"
    );
    ImGui::EndChild();

    ImGui::SameLine();

    // --- R2 ---
    ImGui::BeginChild("R2TriggerPanel", ImVec2(panelWidth, 220.0f), false);
    ImGui::SeparatorText(u8"R2 トリガー");

	ImGui::Text(u8"デッドゾーン");
	ImGui::SetNextItemWidth(-1.0f);
	ImGui::SliderFloat(u8"##RT_DZ", &settings.rightTriggerDeadzone, 0.0f, 1.0f, "%.2f");

	ImGui::Text(u8"判定閾値");
	ImGui::SetNextItemWidth(-1.0f);
	ImGui::SliderFloat(u8"##RT_TH", &settings.rightTriggerThreshold, 0.0f, 1.0f, "%.2f");

    ImGui::Text(u8"Raw: %.3f", ctrl.rightTrigger);

    float rt = applyDZ(ctrl.rightTrigger, settings.rightTriggerDeadzone);
    ImGui::Text(u8"Post: %.3f", rt);

    drawTriggerBar(ctrl.rightTrigger, settings.rightTriggerDeadzone, settings.rightTriggerThreshold);
    ImGui::Text(u8"  赤線 = デッドゾーン ｜ 緑線 = 判定閾値");

    bool r2 = ctrl.rightTrigger >= settings.rightTriggerThreshold;
    ImGui::TextColored(
        r2 ? ImVec4(0.2f, 1.0f, 0.2f, 1.0f) : ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
        u8"R2 判定: %s", r2 ? "ON" : "OFF"
    );
    ImGui::EndChild();
}

// アダプティブトリガータブ
void ControllerToolWindow::DrawAdaptiveTriggerTab(ControllerSettings& settings, const ControllerComponent& ctrl)
{
	if (ctrl.type != ControllerType::PS5)
	{
		ImGui::TextColored({ 0.6f, 0.6f, 0.6f, 1.0f }, u8"PS5 接続時のみ使用可能");
		return;
	}

	// 区間バー（ドラッグで開始・終了位置を決める）
	auto drawRangeBar = [](uint8_t& start, uint8_t& end)
		{
			const float barHeight = 24.0f;
			const float handleHalfWidth = 6.0f;

			ImVec2 barMin = ImGui::GetCursorScreenPos();
			float barWidth = ImGui::GetContentRegionAvail().x;
			ImVec2 barMax = { barMin.x + barWidth, barMin.y + barHeight };

			ImDrawList* dl = ImGui::GetWindowDrawList();

			// 背景
			dl->AddRectFilled(barMin, barMax, IM_COL32(40, 40, 40, 255));
			dl->AddRect(barMin, barMax, IM_COL32(200, 200, 200, 255));

			float startX = barMin.x + (start / 255.0f) * barWidth;
			float endX = barMin.x + (end / 255.0f) * barWidth;

			// 区間の塗りとハンドル
			dl->AddRectFilled({ startX, barMin.y }, { endX, barMax.y }, IM_COL32(80, 160, 220, 120));
			dl->AddRectFilled({ startX - handleHalfWidth, barMin.y - 4 }, { startX + handleHalfWidth, barMax.y + 4 }, IM_COL32(80, 160, 220, 255));
			dl->AddRectFilled({ endX - handleHalfWidth, barMin.y - 4 }, { endX + handleHalfWidth, barMax.y + 4 }, IM_COL32(80, 160, 220, 255));

			// レイアウト上の領域を確保
			ImGui::Dummy({ barWidth, barHeight });

			// 開始ハンドルのドラッグ判定
			ImGui::SetCursorScreenPos({ startX - handleHalfWidth, barMin.y - 4 });
			ImGui::InvisibleButton("##startHandle", { handleHalfWidth * 2.0f, barHeight + 8.0f });
			if (ImGui::IsItemActive())
			{
				float pct = (ImGui::GetMousePos().x - barMin.x) / barWidth;
				pct = std::clamp(pct, 0.0f, 1.0f);
				uint8_t newStart = (uint8_t)(pct * 255.0f);
				if (newStart < end) start = newStart;
			}

			// 終了ハンドルのドラッグ判定
			ImGui::SetCursorScreenPos({ endX - handleHalfWidth, barMin.y - 4 });
			ImGui::InvisibleButton("##endHandle", { handleHalfWidth * 2.0f, barHeight + 8.0f });
			if (ImGui::IsItemActive())
			{
				float pct = (ImGui::GetMousePos().x - barMin.x) / barWidth;
				pct = std::clamp(pct, 0.0f, 1.0f);
				uint8_t newEnd = (uint8_t)(pct * 255.0f);
				if (newEnd > start) end = newEnd;
			}

			// 次のWidget用にカーソル位置を整える
			ImGui::SetCursorScreenPos({ barMin.x, barMax.y + 8.0f });

			ImGui::Text(u8"開始: %d   終了: %d", start, end);
		};

	ImGui::SeparatorText("L2");
	drawRangeBar(settings.leftTriggerEffect.start, settings.leftTriggerEffect.end);

	ImGui::Spacing();

	ImGui::SeparatorText("R2");
	drawRangeBar(settings.rightTriggerEffect.start, settings.rightTriggerEffect.end);


	// カーブグラフ（背景クリックで点を追加、点をドラッグで移動）
	auto drawCurveGraph = [](std::vector<CurvePoint>& points, ImU32 color)
		{
			const float graphHeight = 120.0f;
			ImVec2 graphMin = ImGui::GetCursorScreenPos();
			float graphWidth = ImGui::GetContentRegionAvail().x;
			ImVec2 graphMax = { graphMin.x + graphWidth, graphMin.y + graphHeight };

			ImDrawList* dl = ImGui::GetWindowDrawList();
			dl->AddRectFilled(graphMin, graphMax, IM_COL32(30, 30, 30, 255));
			dl->AddRect(graphMin, graphMax, IM_COL32(120, 120, 120, 255));

			auto toScreen = [&](const CurvePoint& p) -> ImVec2
				{
					return { graphMin.x + p.position * graphWidth, graphMax.y - p.value * graphHeight };
				};
			auto toData = [&](ImVec2 s) -> CurvePoint
				{
					float x = (s.x - graphMin.x) / graphWidth;
					float y = (graphMax.y - s.y) / graphHeight;
					return { std::clamp(x, 0.0f, 1.0f), std::clamp(y, 0.0f, 1.0f) };
				};

			// SampleCurveの前提（position昇順）を保つ
			std::sort(points.begin(), points.end(),
				[](const CurvePoint& a, const CurvePoint& b) { return a.position < b.position; });

			// カーブの線
			for (size_t i = 0; i + 1 < points.size(); ++i)
			{
				dl->AddLine(toScreen(points[i]), toScreen(points[i + 1]), color, 2.0f);
			}

			// 背景クリックで点を追加
			ImGui::SetCursorScreenPos(graphMin);
			ImGui::InvisibleButton("##graphBg", { graphWidth, graphHeight });
			if (ImGui::IsItemClicked())
			{
				points.push_back(toData(ImGui::GetMousePos()));
				std::sort(points.begin(), points.end(),
					[](const CurvePoint& a, const CurvePoint& b) { return a.position < b.position; });
			}

			// 点（ドラッグで移動）
			for (size_t i = 0; i < points.size(); ++i)
			{
				ImVec2 s = toScreen(points[i]);
				ImGui::SetCursorScreenPos({ s.x - 6.0f, s.y - 6.0f });
				ImGui::PushID((int)i);
				ImGui::InvisibleButton("##pt", { 12.0f, 12.0f });
				ImGui::PopID();

				if (ImGui::IsItemActive())
				{
					points[i] = toData(ImGui::GetMousePos());
				}

				dl->AddCircleFilled(s, 5.0f, color);
			}

			ImGui::SetCursorScreenPos({ graphMin.x, graphMax.y + 8.0f });
		};

	ImGui::SeparatorText("L2");
	drawRangeBar(settings.leftTriggerEffect.start, settings.leftTriggerEffect.end);
	ImGui::Text(u8"抵抗力カーブ");
	drawCurveGraph(settings.leftTriggerEffect.forcePoints, IM_COL32(217, 90, 48, 255));
	ImGui::Text(u8"振動カーブ");
	drawCurveGraph(settings.leftTriggerEffect.vibrationPoints, IM_COL32(29, 158, 117, 255));

	ImGui::Spacing();

	ImGui::SeparatorText("R2");
	drawRangeBar(settings.rightTriggerEffect.start, settings.rightTriggerEffect.end);
	ImGui::Text(u8"抵抗力カーブ");
	drawCurveGraph(settings.rightTriggerEffect.forcePoints, IM_COL32(217, 90, 48, 255));
	ImGui::Text(u8"振動カーブ");
	drawCurveGraph(settings.rightTriggerEffect.vibrationPoints, IM_COL32(29, 158, 117, 255));
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
