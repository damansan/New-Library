#pragma once
#include "Core/Scene.h"

#include "ECS/Systems/ControllerSystem.h" 
#include "ECS/Systems/InputSystem.h"    
#include "ECS/Systems/MovementSystem.h" 
#include "ECS/Systems/AnimationSystem.h"

#include "ECS/Systems/MeshLoaderSystem.h"
#include "ECS/Systems/RenderSystem.h"

#include "ECS/Systems/DebugCameraSystem.h"
#include "ECS/Systems/GameCameraSystem.h" 

#include "Rendering/SkyBox.h"
#include "Rendering/LightManager.h"


#include "Tool/AdaptiveTriggerToolWindow.h"

class SceneGame : public Scene
{
public:
	void Initialize(const SceneContext& context) override;
	void Finalize()override {}
	void Update(float elapsedTime, const SceneContext& context) override;
	void Render(const SceneContext& context)override;
	void RenderImGui() override;

private:
	ControllerSystem controllerSystem;
	InputSystem      inputSystem;
	MovementSystem   movementSystem;
	AnimationSystem  animationSystem;
	
	MeshLoaderSystem meshLoaderSystem;
	RenderSystem     renderSystem;
	
	DebugCameraSystem debugCameraSystem;
	GameCameraSystem  gameCameraSystem; 

	AdaptiveTriggerToolWindow adaptiveTriggerToolWindow;

	SkyBox       skyBox;
	LightManager lightManager;
};
