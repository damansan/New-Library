#pragma once
#include "Core/Scene.h"

#include "ECS/Systems/ControllerSystem.h" 
#include "ECS/Systems/GameInputSystem.h"    
#include "ECS/Systems/MovementSystem.h" 
#include "ECS/Systems/AnimationSystem.h"
#include "ECS/Systems/AnimatorControllerSystem.h"


#include "ECS/Systems/PlayerMoveSystem.h"


#include "ECS/Systems/MeshLoaderSystem.h"
#include "ECS/Systems/RenderSystem.h"

#include "ECS/Systems/DebugCameraSystem.h"
#include "ECS/Systems/GameCameraSystem.h" 

#include "Rendering/SkyBox.h"
#include "Rendering/LightManager.h"
#include "Rendering/PhysicsDebugRenderer.h"

#include "ECS/Systems/PhysicsSystem.h"
#include "ECS/Systems/IKSystem.h"

#include "ECS/Systems/SequencerSystem.h"

#include "Tool/AdaptiveTriggerToolWindow.h"


#include "Demo/WaveDemo.h"

class SceneGame : public Scene
{
public:
	void Initialize(const SceneContext& context) override;
	void Finalize()override;
	void Update(float elapsedTime, const SceneContext& context) override;
	void Render(const SceneContext& context)override;
	void RenderImGui() override;

private:
	ControllerSystem controllerSystem;
	GameInputSystem  gameInputSystem;
	MovementSystem   movementSystem;
	AnimationSystem  animationSystem;
	AnimatorControllerSystem animatorControllerSystem;
	PlayerMoveSystem playerMoveSystem;

	MeshLoaderSystem meshLoaderSystem;
	RenderSystem     renderSystem;

	DebugCameraSystem debugCameraSystem;
	GameCameraSystem  gameCameraSystem;

	AdaptiveTriggerToolWindow adaptiveTriggerToolWindow;

	SkyBox       skyBox;
	LightManager lightManager;
	PhysicsDebugRenderer physicsDebugRenderer;

	PhysicsSystem    physicsSystem;

	IKSystem         ikSystem;

	SequencerSystem sequencerSystem;

private:
	// ƒfƒ‚—p
	WaveDemo waveDemo;
};
