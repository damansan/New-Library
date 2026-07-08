#pragma once
#include "NodeEditorCanvas.h"

class AnimatorGraphEditor
{
public:
    void Initialize();
    void DrawGUI(const char* windowName = "Animator");

private:
    NodeEditorCanvas canvas;
};