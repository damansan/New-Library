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
