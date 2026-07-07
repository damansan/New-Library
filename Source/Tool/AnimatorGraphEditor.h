#pragma once
#include <imgui_node_editor.h>

namespace ed = ax::NodeEditor;

class AnimatorGraphEditor
{
public:
    void Initialize();
    void Shutdown();
    void DrawGUI(const char* windowName = "Animator");

private:
    ed::EditorContext* editorContext = nullptr;
};