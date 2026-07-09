#pragma once
#include "NodeEditorCanvas.h"

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
    NodeEditorCanvas canvas;
};