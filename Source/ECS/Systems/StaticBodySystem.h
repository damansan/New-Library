#pragma once
#include "entt.hpp"
#include "Core/PhysX.h"

namespace physx { class PxScene; }

class StaticBodySystem
{
public:
	void Update(entt::registry& registry, PhysX& physX, physx::PxScene* pxScene);
};