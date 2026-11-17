#pragma once
#include "entt.hpp"
#include "Game/ECS/Components/CameraEffectComponent.h"
#include "Game/ECS/Components/GameCameraComponent.h"

class CameraEffectSystem
{
public:
	void Update(entt::registry& registry, float elapsedTime);

private:
	// ë¨Ç≥Ç≈ FOV ÇçLÇ∞ÇÈ
	void FovEffect(entt::registry& registry, CameraEffectComponent& effect, entt::entity player, float elapsedTime);

};