#pragma once
#include <d3d11.h>
#include <algorithm>
#include "entt.hpp"
#include "Engine/Resource/Model.h" 
#include "Engine/ECS/Components/AnimatorComponent.h"
#include "Engine/ECS/Components/SkinnedMeshComponent.h"

class AnimationSystem
{
public:
	// 更新（時間を進めてボーン行列を計算）
	void Update(
		entt::registry& registry,
		float elapsedTime);

	// 時間を進めずに、指定した時刻のポーズだけ計算する
	void EvaluatePose(entt::registry& registry);

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
		std::vector<DirectX::XMFLOAT3>& outPositions,
		std::vector<DirectX::XMFLOAT4>& outRotations,
		std::vector<DirectX::XMFLOAT3>& outScales,
		int rootMotionNodeIndex = -1);

	// 2つのポーズをrateで補間する
	static void BlendPoses(
		const std::vector<DirectX::XMFLOAT3>& posA, const std::vector<DirectX::XMFLOAT4>& rotA, const std::vector<DirectX::XMFLOAT3>& scaleA,
		const std::vector<DirectX::XMFLOAT3>& posB, const std::vector<DirectX::XMFLOAT4>& rotB, const std::vector<DirectX::XMFLOAT3>& scaleB,
		float rate,
		std::vector<DirectX::XMFLOAT3>& outPositions,
		std::vector<DirectX::XMFLOAT4>& outRotations,
		std::vector<DirectX::XMFLOAT3>& outScales);

	// 名前からルートノードインデックスを探す
	static int FindNodeIndex(const Model& model, const std::string& name);

	// 指定時刻のローカル位置を求める
	static DirectX::XMFLOAT3 CalcNodeLocalPosition(
		const Model::Animation& animation,
		int nodeIndex,
		const Model::Node& node,
		float time);

	// ルートモーションの移動量を計算する
	static DirectX::XMFLOAT3 CalcRootMotionDelta(
		const Model& model,
		const Model::Animation& animation,
		int rootMotionNodeIndex,
		float oldTime,
		float newTime,
		float rangeStart = 0.0f,
		float rangeEnd = -1.0f);
};