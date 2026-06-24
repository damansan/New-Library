#pragma once
#include "entt.hpp"
#include "Core/PhysX.h"

namespace physx { class PxScene; }

class PhysicsSystem
{
public:
	// XV
	void Update(entt::registry& registry, PhysX& physX, physx::PxScene* pxScene, physx::PxControllerManager* controllerManager, float elapsedTime);

private:
	// íœ—\–ñ‚³‚ê‚½Actor‚ğ‰ğ•ú‚·‚é
	void ProcessDestroyQueue();

	// –¢¶¬‚ÌStaticBodyComponent¶¬
	void ProcessStaticBodies(entt::registry& registry, PhysX& physX, physx::PxScene* pxScene);

	// –¢¶¬‚ÌCharacterControllerComponent¶¬+CharacterControllerˆÚ“®ˆ—
	void ProcessCharacterControllers(entt::registry& registry, PhysX& physX, physx::PxControllerManager* controllerManager, float elapsedTime);

	// –¢¶¬‚Ì
	// void ProcessDynamicBodies(entt::registry& registry, PhysX& physX, physx::PxScene* pxScene);
};