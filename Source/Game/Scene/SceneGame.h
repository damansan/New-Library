#pragma once
#include "Engine/Core/Scene.h"

// ゲームプレイ系
#include "Engine/ECS/Systems/ControllerSystem.h"
#include "Game/ECS/Systems/GameInputSystem.h"
#include "Engine/ECS/Systems/TransitionControllerSystem.h"

#include "Game/ECS/Systems/PlayerIdleSystem.h"
#include "Game/ECS/Systems/PlayerMoveSystem.h"
#include "Game/ECS/Systems/PlayerDodgeSystem.h"
#include "Game/ECS/Systems/PlayerSprintSystem.h"
#include "Game/ECS/Systems/PlayerJumpSystem.h"

#include "Engine/ECS/Systems/MovementSystem.h"
#include "Engine/ECS/Systems/AnimatorControllerSystem.h"
#include "Engine/ECS/Systems/AnimationSpeedSystem.h"
#include "Engine/ECS/Systems/AnimationSystem.h"
#include "Engine/ECS/Systems/SequencerSystem.h"
#include "Engine/ECS/Systems/IKSystem.h"
#include "Engine/ECS/Systems/MeshLoaderSystem.h"
#include "Engine/ECS/Systems/PhysicsSystem.h"
#include "Game/ECS/Systems/GameCameraSystem.h"

// 描画系
#include "Engine/ECS/Systems/RenderSystem.h"
#include "Engine/Rendering/SkyBox.h"
#include "Engine/Rendering/LightManager.h"
#include "Engine/Rendering/PhysicsDebugRenderer.h"

// デモ
#include "Game/Demo/WaveDemo.h"

class SceneGame : public Scene
{
public:
	void Initialize(const SceneContext& context) override;
	void Finalize()override;
	void Update(float elapsedTime, const SceneContext& context) override;
	void Render(const SceneContext& context)override;

private:
	void UpdateGameplay(float elapsedTime, const SceneContext& context);  // ゲームプレイ系
	void UpdateAnimation(float elapsedTime, const SceneContext& context); // アニメーション系
	void UpdateResources(float elapsedTime, const SceneContext& context); // リソース系
	void UpdateCamera(float elapsedTime, const SceneContext& context);    // カメラ

	// ゲームプレイ系
	ControllerSystem  controllerSystem;
	GameInputSystem   gameInputSystem;
	TransitionControllerSystem transitionControllerSystem;
	 
	PlayerIdleSystem   playerIdleSystem;
	PlayerMoveSystem   playerMoveSystem;
	PlayerDodgeSystem  playerDodgeSystem;
	PlayerSprintSystem playerSprintSystem;
	PlayerJumpSystem   playerJumpSystem;

	MovementSystem    movementSystem;
	AnimatorControllerSystem animatorControllerSystem;
	AnimationSpeedSystem     animationSpeedSystem;
	AnimationSystem   animationSystem;
	SequencerSystem   sequencerSystem;
	IKSystem          ikSystem;
	MeshLoaderSystem  meshLoaderSystem;
	PhysicsSystem     physicsSystem;
	GameCameraSystem  gameCameraSystem;

	// 描画系
	RenderSystem         renderSystem;
	SkyBox                skyBox;
	LightManager          lightManager;
	PhysicsDebugRenderer  physicsDebugRenderer;

private:
	// デモ用
	WaveDemo waveDemo;

	// デバッグ描画切り替えフラグ
	bool DrawDebug = true;
};
