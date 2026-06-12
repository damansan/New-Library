#pragma once
#include "Core/Scene.h"
#include "ECS/Systems/RenderSystem.h"
#include "ECS/Systems/AnimationSystem.h"
#include "ECS/Systems/InputSystem.h"    
#include "ECS/Systems/MovementSystem.h" 
#include "ECS/Systems/DebugCameraSystem.h"
#include "ECS/Systems/GameCameraSystem.h" 

#include "DebugRenderer/DebugRenderer.h"

#include "Rendering/SkyBox.h"
#include "Rendering/LightManager.h"

#include <memory>

#include "Game/Stage.h"
#include "Game/Player.h"
#include "Game/Enemy.h"

#include "Game/DamagedHelmet.h"

class SceneGame : public Scene
{
public:
	void Initialize(const SceneContext& context) override;
	void Finalize()override {}
	void Update(float elapsedTime, const SceneContext& context) override;
	void Render(const SceneContext& context)override;
	void RenderImGui() override;

private:
	RenderSystem renderSystem;
	AnimationSystem animationSystem;
	InputSystem     inputSystem;
	MovementSystem  movementSystem;
	DebugCameraSystem debugCameraSystem;
	GameCameraSystem  gameCameraSystem; 

	DebugRenderer debugRenderer;

	SkyBox skyBox;
	LightManager lightManager;

	// ステージ
	std::unique_ptr<Stage> stage;
	// プレイヤー
	std::unique_ptr<Player> player;
	// エネミー
	std::unique_ptr<Enemy> enemy;
};
