#pragma once
#include "entt.hpp"
#include "Engine/Core/Camera.h"

class CameraViewSystem
{
public:
	// ÅI‚Ì’l‚ğì‚é
	void Update(entt::registry& registry);

	// Camera ‚É“n‚·
	void ApplyToCamera(entt::registry& registry, Camera& camera);
};