#pragma once
#include "IStudioAttachment.h"
#include "ECS/Components/AnimatorComponent.h"
#include "ECS/Components/SkinnedMeshComponent.h"

class StudioTimeline : public IStudioAttachment
{
public:
    StudioTimeline() = default;
    ~StudioTimeline() = default;

    void SetWindowId(int id) { windowId = id; }

    void DrawGUI(
        entt::registry& registry,
        StudioContext& context) override;

private:
    int windowId = 0;
};