#pragma once
#include "Engine/Core/Scene.h"

#include "Engine/ECS/Systems/ControllerSystem.h" 
#include "Project/ECS/Systems/GameInputSystem.h"    
#include "Engine/ECS/Systems/MovementSystem.h" 
#include "Engine/ECS/Systems/AnimationSystem.h"
#include "Engine/ECS/Systems/AnimatorControllerSystem.h"


#include "Project/ECS/Systems/PlayerMoveSystem.h"


#include "Engine/ECS/Systems/MeshLoaderSystem.h"
#include "Engine/ECS/Systems/RenderSystem.h"

#include "Engine/ECS/Systems/DebugCameraSystem.h"
#include "Project/ECS/Systems/GameCameraSystem.h" 

#include "Engine/Rendering/SkyBox.h"
#include "Engine/Rendering/LightManager.h"
#include "Engine/Rendering/PhysicsDebugRenderer.h"

#include "Engine/ECS/Systems/PhysicsSystem.h"
#include "Engine/ECS/Systems/IKSystem.h"

#include "Engine/ECS/Systems/SequencerSystem.h"

#include "Tool/AdaptiveTriggerToolWindow.h"


#include "Unsorted/Demo/WaveDemo.h"

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
