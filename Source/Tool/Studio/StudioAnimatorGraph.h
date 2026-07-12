#pragma once
#include "IStudioAttachment.h"
#include "Tool/AnimatorGraph.h"
#include "Tool/NodeEditorCanvas.h"

class StudioAnimatorGraph : public IStudioAttachment
{
public:
	StudioAnimatorGraph() = default;
	~StudioAnimatorGraph() = default;

	void Initialize();
	void SetWindowId(int id) { windowId = id; }

	// ImGui描画
	void DrawGUI(
		entt::registry& registry,
		StudioContext& context) override;

private:
	NodeEditorCanvas canvas;
	AnimatorGraph graph; // 編集対象データ
	int windowId = 0;
};