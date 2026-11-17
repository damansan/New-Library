#pragma once
#include "entt.hpp"
#include "Game/ECS/Components/CameraEffectComponent.h"
#include "Game/ECS/Components/GameCameraComponent.h"

// ‰‰o‚Ì‚«‚Á‚©‚¯
struct CameraEffectTrigger
{
	float speed = 0.0f;    // …•½‚Ì‘¬‚³
	bool  running = false; // ‘–‚Á‚Ä‚¢‚é(runSpeed ˆÈã)

	bool wallKicked = false;    // •Ç‚ğR‚Á‚½
	bool kickWallRight = false; // R‚Á‚½•Ç‚ª‰E‚É‚ ‚é‚©
};


class CameraEffectSystem
{
public:
	void Update(entt::registry& registry, float elapsedTime);

private:
	// ƒvƒŒƒCƒ„[‚ğŒ©‚ÄA‚«‚Á‚©‚¯‚ğì‚é
	CameraEffectTrigger BuildTrigger(entt::registry& registry, const CameraEffectComponent& effect, entt::entity player);

	// ‘¬‚³‚Å FOV ‚ğL‚°‚é
	void FovEffect(CameraEffectComponent& effect, const CameraEffectTrigger& trigger, float elapsedTime);
	// ‘–‚èo‚µ‚É¶‰E‚Ö—h‚ç‚·
	void RunBobEffect(CameraEffectComponent& effect, const CameraEffectTrigger& trigger, const GameCameraComponent& cam, float elapsedTime);
	// •ÇR‚è‚Å¬‚³‚­—h‚ç‚·
	void KickShakeEffect(CameraEffectComponent& effect, const CameraEffectTrigger& trigger, const GameCameraComponent& cam, float elapsedTime);
};