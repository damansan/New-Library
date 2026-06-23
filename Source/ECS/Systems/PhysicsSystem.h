#pragma once
#include "entt.hpp"
#include "Core/PhysX.h"

namespace physx { class PxScene; }

class PhysicsSystem
{
public:
	// XV
	void Update(entt::registry& registry, PhysX& physX, physx::PxScene* pxScene);

private:
	// íœ—\–ñ‚³‚ê‚½Actor‚ğ‰ğ•ú‚·‚é
	void ProcessDestroyQueue();

	// –¢¶¬‚ÌStaticBodyComponent¶¬
	void ProcessStaticBodies(entt::registry& registry, PhysX& physX, physx::PxScene* pxScene);

	// –¢¶¬‚Ì
	// void ProcessDynamicBodies(entt::registry& registry, PhysX& physX, physx::PxScene* pxScene);
	// –¢¶¬‚Ì
	// void ProcessCharacterControllers(entt::registry& registry, PhysX& physX, physx::PxScene* pxScene);
};