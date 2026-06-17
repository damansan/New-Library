#pragma once
#include "IStudioAttachment.h"
#include "ECS/Components/AnimatorComponent.h"
#include "ECS/Components/SkinnedMeshComponent.h"

class StudioAnimator : public IStudioAttachment
{
public:
    StudioAnimator() = default;
    ~StudioAnimator() = default;

    void SetWindowId(int id) { windowId = id; }

    // ImGui•`‰æ
    void DrawGUI(
        entt::registry& registry,
        StudioContext& context) override;

private:
    int windowId = 0;
};