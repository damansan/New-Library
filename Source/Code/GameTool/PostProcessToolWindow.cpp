#include "PostProcessToolWindow.h"
#include <imgui.h>
#include <string>

// 調整画面を描く
void PostProcessToolWindow::Draw(PostProcessSettings& settings)
{
	if (!isOpen) return;

	if (!ImGui::Begin("Post Process", &isOpen))
	{
		ImGui::End();
		return;
	}

	// トーンマップ
	ImGui::SeparatorText("Tone Mapping");
	ImGui::SliderFloat("Exposure", &settings.exposure, 0.0f, 4.0f, "%.2f");
	ImGui::Checkbox("ACES", &settings.tonemapEnabled);

	// カラー調整
	ImGui::SeparatorText("Color Adjustments");
	ImGui::SliderFloat("Contrast", &settings.contrast, 0.0f, 2.0f, "%.2f");
	ImGui::ColorEdit3("Color Filter", &settings.colorFilter.x);
	ImGui::SliderFloat("Saturation", &settings.saturation, 0.0f, 2.0f, "%.2f");


	ImGui::Spacing();
	ImGui::Separator();

	// 初期値に戻す
	if (ImGui::Button("Reset"))
	{
		settings = PostProcessSettings{};
	}
	ImGui::SameLine();

	ImGui::End();
}
