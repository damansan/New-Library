#pragma once
#include <PxPhysicsAPI.h>
#include "Core/PhysicsFilter.h"

class WalkableQueryFilter : public physx::PxQueryFilterCallback
{
public:
	physx::PxQueryHitType::Enum preFilter(
		const physx::PxFilterData& filterData, const physx::PxShape* shape,
		const physx::PxRigidActor* actor, physx::PxHitFlags& queryFlags) override
	{
		// トリガーシェイプは地面として扱わない
		if (shape->getFlags() & physx::PxShapeFlag::eTRIGGER_SHAPE)
		{
			return physx::PxQueryHitType::eNONE;
		}

		// Walkable属性が立っていないシェイプは無視（自キャラ自身のカプセルもここで除外される）
		physx::PxFilterData queryFilterData = shape->getQueryFilterData();
		if (!(queryFilterData.word2 & static_cast<physx::PxU32>(PhysicsAttribute::Walkable)))
		{
			return physx::PxQueryHitType::eNONE;
		}

		return physx::PxQueryHitType::eBLOCK;
	}

	physx::PxQueryHitType::Enum postFilter(
		const physx::PxFilterData& filterData, const physx::PxQueryHit& hit,
		const physx::PxShape* shape, const physx::PxRigidActor* actor) override
	{
		return physx::PxQueryHitType::eBLOCK;
	}
};