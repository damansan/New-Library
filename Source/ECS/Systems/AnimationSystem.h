#pragma once
#include <d3d11.h>
#include "entt.hpp"
#include "Resource/Model.h" 
#include "ECS/Components/AnimatorComponent.h"
#include "ECS/Components/SkinnedMeshComponent.h"

class AnimationSystem
{
public:
	// 更新（時間を進めてボーン行列を計算）
	void Update(
		entt::registry& registry,
		float elapsedTime);

	// 時間を進めずに、指定した時刻のポーズだけ計算する
	void AnimationSystem::EvaluatePose(entt::registry& registry);

private:
	// キーフレーム補間（位置）
	static DirectX::XMFLOAT3 InterpolatePosition(
		const std::vector<Model::VectorKeyframe>& keyframes,
		float time);

	// キーフレーム補間（回転）
	static DirectX::XMFLOAT4 InterpolateRotation(
		const std::vector<Model::QuaternionKeyframe>& keyframes,
		float time);

	// キーフレーム補間（スケール）
	static DirectX::XMFLOAT3 InterpolateScale(
		const std::vector<Model::VectorKeyframe>& keyframes,
		float time);

	// ノードのグローバル行列を再帰計算してボーン行列に書き込む
	static void CalcBoneTransforms(
		Model& model,
		const Model::Animation& animation,
		float time,
		SkinnedMeshComponent& skinned);
};