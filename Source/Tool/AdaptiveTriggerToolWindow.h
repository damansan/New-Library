#pragma once
#include <vector>
#include "ECS/Components/ControllerComponent.h"

class AdaptiveTriggerToolWindow
{
public:
    void Draw(ControllerComponent& ctrl);

private:
    // ゾーンの状態（ポイント配列とは同期させない、独立したテスト用データ）
    struct ZoneState
    {
        bool  exists = false;
        float start = 0.0f;
        float end = 1.0f;
        float force = 0.5f;
    };

    struct RangeDragState
    {
        enum class Mode { None, Left, Right, Move };
        Mode mode = Mode::None;
        float startMouseX = 0.0f;
        float startZoneStart = 0.0f;
        float startZoneEnd = 0.0f;
    };

    void DrawTriggerSection(const char* label, AdaptiveTrigger::TriggerEffect& effect, ZoneState& zone, RangeDragState& rangeDrag);
    void DrawRangeBar(ZoneState& zone, float width, RangeDragState& rangeDrag);

    ZoneState leftZone;
    ZoneState rightZone;
    RangeDragState leftRangeDrag;
    RangeDragState rightRangeDrag;
};