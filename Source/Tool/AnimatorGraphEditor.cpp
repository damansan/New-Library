#include "AnimatorGraphEditor.h"
#include "imgui.h"

AnimatorGraphEditor::~AnimatorGraphEditor()
{
    if (editorContext)
    {
        ed::DestroyEditor(editorContext);
        editorContext = nullptr;
    }
}

void AnimatorGraphEditor::Initialize()
{
	ed::Config config;
	editorContext = ed::CreateEditor(&config);
}


void AnimatorGraphEditor::DrawGUI(const char* windowName)
{
    ImGui::Begin(windowName);

    ed::SetCurrentEditor(editorContext);
    ed::Begin("AnimatorCanvas");


    ed::End();
    ed::SetCurrentEditor(nullptr);

    ImGui::End();
}