#pragma once
#include "Core/Scene.h"
#include "ECS/Systems/RenderSystem.h"

class SceneGame : public Scene
{
public:
	void Initialize(const SceneContext& context) override;
	void Finalize()override {}
	void Update(float elapsedTime, const SceneContext& context) override;
	void Render(const SceneContext& context)override;
	void RenderImGui() override {}

private:
	RenderSystem renderSystem;
};
