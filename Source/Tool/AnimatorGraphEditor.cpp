#include "AnimatorGraphEditor.h"
#include "imgui.h"

void AnimatorGraphEditor::Initialize()
{
    canvas.Initialize();
}

void AnimatorGraphEditor::DrawGUI(const char* windowName)
{
    ImGui::Begin(windowName);

    canvas.Begin("AnimatorCanvas");

    canvas.End();

    ImGui::End();
}