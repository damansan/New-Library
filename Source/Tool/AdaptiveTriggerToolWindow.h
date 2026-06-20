#pragma once
#include "ECS/Components/ControllerComponent.h"

class AdaptiveTriggerToolWindow
{
public:
    void Draw(ControllerComponent& ctrl);

private:
    // L2/R2共通のカーブ編集処理（1つの関数を2回呼ぶことで重複を避ける）
    void DrawTriggerCurve(const char* label, AdaptiveTrigger::TriggerEffect& effect, float currentValue, int& draggingIndex);

    // ドラッグ中のポイントのインデックス（ドラッグしていない時は-1）
    int leftDraggingIndex = -1;
    int rightDraggingIndex = -1;
};