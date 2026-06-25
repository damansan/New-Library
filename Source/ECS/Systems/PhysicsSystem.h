#pragma once
#include "entt.hpp"
#include "Core/PhysX.h"

namespace physx { class PxScene; }

class PhysicsSystem
{
public:
	// 更新
	void Update(entt::registry& registry, PhysX& physX, physx::PxScene* pxScene, physx::PxControllerManager* controllerManager, float elapsedTime);

private:
	// 削除予約されたActorを解放する
	void ProcessDestroyQueue();

	// 未生成のStaticBodyComponent生成
	void ProcessStaticBodies(entt::registry& registry, PhysX& physX, physx::PxScene* pxScene);

	// 未生成のCharacterControllerComponent生成+CharacterController移動処理
	void ProcessCharacterControllers(entt::registry& registry, PhysX& physX, physx::PxControllerManager* controllerManager, float elapsedTime);
	// 足元判定サポート
	void ProcessEdgeSlide(entt::registry& registry, physx::PxScene* pxScene);

	// 未生成の
	// void ProcessDynamicBodies(entt::registry& registry, PhysX& physX, physx::PxScene* pxScene);
};
