#pragma once
#include <imgui_node_editor.h>

namespace ed = ax::NodeEditor;

class AnimatorGraphEditor
{
public:
    AnimatorGraphEditor() = default;
    ~AnimatorGraphEditor();
    AnimatorGraphEditor(const AnimatorGraphEditor&) = delete;
    AnimatorGraphEditor& operator=(const AnimatorGraphEditor&) = delete;

    void Initialize();
    void DrawGUI(const char* windowName = "Animator");

private:
    ed::EditorContext* editorContext = nullptr;
};