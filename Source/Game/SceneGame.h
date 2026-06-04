#pragma once
#include "Core/Scene.h"
#include "ECS/Systems/RenderSystem.h"
#include "ECS/Systems/AnimationSystem.h"
#include "Debug/DebugRenderer.h"
#include <memory>

#include "Game/Stage.h"
#include "Game/Player.h"


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
	AnimationSystem animationSystem;
	DebugRenderer debugRenderer;

	// ステージ
	std::unique_ptr<Stage> stage;
	// プレイヤー
	std::unique_ptr<Player> player;
};
