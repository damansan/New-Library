#pragma once
#include <PxPhysicsAPI.h>

struct StaticBodyComponent
{
    physx::PxRigidStatic* actor = nullptr;
};