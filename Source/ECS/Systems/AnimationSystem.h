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
		const Model& model,
		const Model::Animation& animation,
		float time,
		std::vector<DirectX::XMFLOAT4X4>& boneTransforms);
};