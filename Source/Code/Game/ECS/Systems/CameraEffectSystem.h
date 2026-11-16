#pragma once
#pragma once
#include "entt.hpp"
#include "Engine/Core/Camera.h"

#include "Game/ECS/Components/CameraEffectComponent.h"
#include "Game/ECS/Components/GameCameraComponent.h"

class CameraEffectSystem
{
public:
	// ‰‰o‚ğŒˆ‚ß‚é
	void Update(entt::registry& registry, float elapsedTime);

	// Camera ‚É”½‰f‚·‚é
	void ApplyToCamera(entt::registry& registry, Camera& camera);

private:

	// ‘¬‚³‚Å Fov ‚ğL‚°‚é
	void FovEffect(entt::registry& registry, CameraEffectComponent& effect, entt::entity player, float elapsedTime);

};