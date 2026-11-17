#pragma once
#include "entt.hpp"
#include "Game/ECS/Components/CameraEffectComponent.h"
#include "Game/ECS/Components/GameCameraComponent.h"

// 演出のきっかけ
struct CameraEffectTrigger
{
	float speed = 0.0f;    // 水平の速さ
	bool  running = false; // 走っている(runSpeed 以上)

	bool wallKicked = false;    // 壁を蹴った
	bool kickWallRight = false; // 蹴った壁が右にあるか
};


class CameraEffectSystem
{
public:
	void Update(entt::registry& registry, float elapsedTime);

private:
	// プレイヤーを見て、きっかけを作る
	CameraEffectTrigger BuildTrigger(entt::registry& registry, const CameraEffectComponent& effect, entt::entity player);

	// 速さで FOV を広げる
	void FovEffect(CameraEffectComponent& effect, const CameraEffectTrigger& trigger, float elapsedTime);
	// 走り出しに左右へ揺らす
	void RunBobEffect(CameraEffectComponent& effect, const CameraEffectTrigger& trigger, const GameCameraComponent& cam, float elapsedTime);
};