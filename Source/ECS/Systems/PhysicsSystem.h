#pragma once
#include "entt.hpp"
#include "Core/PhysX.h"

#include <DirectXMath.h>

namespace physx { class PxScene; }

// レイキャストの結果
struct RaycastHitResult
{
	bool               hit = false;      // 当たったかどうか
	DirectX::XMFLOAT3  position{};       // 当たった座標
	DirectX::XMFLOAT3  normal{};         // 当たった面の法線
	float              distance = 0.0f;  // 開始位置からの距離
};

class PhysicsSystem
{
public:
	// 更新
	void Update(entt::registry& registry, PhysX& physX, physx::PxScene* pxScene, physx::PxControllerManager* controllerManager, float elapsedTime);

	// レイヤーにレイキャスト
	static RaycastHitResult Raycast(
		physx::PxScene* pxScene,
		const DirectX::XMFLOAT3& origin,
		const DirectX::XMFLOAT3& direction,
		float maxDistance,
		physx::PxU32 wantedLayers);

private:
	// 削除予約されたActorを解放する
	void ProcessDestroyQueue();

	// 未生成のStaticBodyComponent生成
	void ProcessStaticBodies(entt::registry& registry, PhysX& physX, physx::PxScene* pxScene);

	// 未生成のCharacterControllerComponent生成+CharacterController移動処理
	void ProcessCharacterControllers(entt::registry& registry, PhysX& physX, physx::PxControllerManager* controllerManager, float elapsedTime);

	// 未生成の
	// void ProcessDynamicBodies(entt::registry& registry, PhysX& physX, physx::PxScene* pxScene);
};