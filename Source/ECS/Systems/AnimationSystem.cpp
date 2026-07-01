#include "AnimationSystem.h"
#include "tracy/Tracy.hpp"
#include "Resource/SkeletonUtils.h"

// 更新
void AnimationSystem::Update(
	entt::registry& registry,
	float elapsedTime)
{
	ZoneScoped;

	auto view = registry.view<AnimatorComponent, SkinnedMeshComponent>();
	view.each(
		[&](auto entity,
			AnimatorComponent& animator,
			SkinnedMeshComponent& skinned)
		{
			ZoneScopedN("AnimationSystem::Entity");

			// モデルが無効なら何もしない
			if (!skinned.model) return;
			if (!animator.isPlaying) return;

			const auto& animations = skinned.model->GetAnimations();
			if (animations.empty()) return;

			// アニメーション名からインデックスを探す
			int animIndex = -1;
			for (int i = 0; i < (int)animations.size(); i++)
			{
				if (animations[i].name == animator.currentAnimation)
				{
					animIndex = i;
					break;
				}
			}

			// 見つからなければ0番目を使う
			if (animIndex < 0)
			{
				animIndex = 0;
				animator.currentAnimation = animations[0].name;
			}

			const auto& animation = animations[animIndex];
			animator.animationTime += elapsedTime * animator.playSpeed;

			// ループ処理
			if (animator.loop)
			{
				if (animator.animationTime > animation.secondsLength)
				{
					animator.animationTime -= animation.secondsLength;
					animator.isEnd = false;
				}
			}
			else
			{
				if (animator.animationTime >= animation.secondsLength)
				{
					animator.animationTime = animation.secondsLength;
					animator.isPlaying = false;
					animator.isEnd = true;
				}
			}

			// ボーン行列を計算
			CalcBoneTransforms(
				*skinned.model,
				animation,
				animator.animationTime,
				skinned);
		});
}

// 時間を進めずに、指定した時刻のポーズだけ計算する
void AnimationSystem::EvaluatePose(entt::registry& registry)
{
	ZoneScoped;

	auto view = registry.view<AnimatorComponent, SkinnedMeshComponent>();
	view.each(
		[&](auto entity,
			AnimatorComponent& animator,
			SkinnedMeshComponent& skinned)
		{
			if (!skinned.model) return;

			const auto& animations = skinned.model->GetAnimations();
			if (animations.empty()) return;

			int animIndex = -1;
			for (int i = 0; i < (int)animations.size(); i++)
			{
				if (animations[i].name == animator.currentAnimation)
				{
					animIndex = i;
					break;
				}
			}
			if (animIndex < 0) return;

			const auto& animation = animations[animIndex];

			// ボーン行列だけ計算（時間は進めない）
			CalcBoneTransforms(
				*skinned.model,
				animation,
				animator.animationTime,
				skinned);
		});
}

// キーフレーム補間（位置）
DirectX::XMFLOAT3 AnimationSystem::InterpolatePosition(
	const std::vector<Model::VectorKeyframe>& keyframes,
	float time)
{
	if (keyframes.size() == 1)
	{
		return keyframes[0].value;
	}

	// 前後のキーフレームを探す
	for (int i = 0; i < (int)keyframes.size() - 1; i++)
	{
		if (time <= keyframes[i + 1].seconds)
		{
			float t = (time - keyframes[i].seconds) / (keyframes[i + 1].seconds - keyframes[i].seconds);

			DirectX::XMVECTOR a = DirectX::XMLoadFloat3(
				&keyframes[i].value);
			DirectX::XMVECTOR b = DirectX::XMLoadFloat3(
				&keyframes[i + 1].value);

			DirectX::XMFLOAT3 result;
			DirectX::XMStoreFloat3(
				&result,
				DirectX::XMVectorLerp(a, b, t));
			return result;
		}
	}
	// 末尾を返す
	return keyframes.back().value;
}

// キーフレーム補間（回転）
DirectX::XMFLOAT4 AnimationSystem::InterpolateRotation(
	const std::vector<Model::QuaternionKeyframe>& keyframes,
	float time)
{
	if (keyframes.size() == 1)
	{
		return keyframes[0].value;
	}

	for (int i = 0; i < (int)keyframes.size() - 1; i++)
	{
		if (time <= keyframes[i + 1].seconds)
		{
			float t = (time - keyframes[i].seconds) / (keyframes[i + 1].seconds - keyframes[i].seconds);

			DirectX::XMVECTOR a = DirectX::XMLoadFloat4(
				&keyframes[i].value);
			DirectX::XMVECTOR b = DirectX::XMLoadFloat4(
				&keyframes[i + 1].value);

			DirectX::XMFLOAT4 result;
			DirectX::XMStoreFloat4(
				&result,
				DirectX::XMQuaternionSlerp(a, b, t));
			return result;
		}
	}
	return keyframes.back().value;
}

// キーフレーム補間（スケール）
DirectX::XMFLOAT3 AnimationSystem::InterpolateScale(
	const std::vector<Model::VectorKeyframe>& keyframes,
	float time)
{
	if (keyframes.size() == 1)
	{
		return keyframes[0].value;
	}

	for (int i = 0; i < (int)keyframes.size() - 1; i++)
	{
		if (time <= keyframes[i + 1].seconds)
		{
			float t = (time - keyframes[i].seconds) / (keyframes[i + 1].seconds - keyframes[i].seconds);

			DirectX::XMVECTOR a = DirectX::XMLoadFloat3(
				&keyframes[i].value);
			DirectX::XMVECTOR b = DirectX::XMLoadFloat3(
				&keyframes[i + 1].value);

			DirectX::XMFLOAT3 result;
			DirectX::XMStoreFloat3(
				&result,
				DirectX::XMVectorLerp(a, b, t));
			return result;
		}
	}
	return keyframes.back().value;
}

// ノードのグローバル行列を再帰計算してボーン行列に書き込む
void AnimationSystem::CalcBoneTransforms(
	Model& model,
	const Model::Animation& animation,
	float time,
	SkinnedMeshComponent& skinned)
{
	auto& nodes = model.GetNodes();

	// ローカルpose配列をノード数に合わせてリサイズ
	skinned.localPositions.resize(nodes.size());
	skinned.localRotations.resize(nodes.size());
	skinned.localScales.resize(nodes.size());

	for (int i = 0; i < (int)nodes.size(); i++)
	{
		skinned.localPositions[i] = nodes[i].position;
		skinned.localRotations[i] = nodes[i].rotation;
		skinned.localScales[i] = nodes[i].scale;

		// キーフレームがあれば補間して上書き
		if (i < (int)animation.nodeAnims.size())
		{
			const auto& nodeAnim = animation.nodeAnims[i];

			if (!nodeAnim.positionKeyframes.empty())
			{
				skinned.localPositions[i] = InterpolatePosition(
					nodeAnim.positionKeyframes, time);
			}

			if (!nodeAnim.rotationKeyframes.empty())
			{
				skinned.localRotations[i] = InterpolateRotation(
					nodeAnim.rotationKeyframes, time);
			}

			if (!nodeAnim.scaleKeyframes.empty())
			{
				skinned.localScales[i] = InterpolateScale(
					nodeAnim.scaleKeyframes, time);
			}
		}
	}

	// 階層計算とスキニング行列化は共通ロジック(SkeletonUtils)に任せる
	SkeletonUtils::CalcGlobalTransforms(
		nodes, skinned.localPositions, skinned.localRotations, skinned.localScales,
		skinned.nodeGlobalTransforms);

	SkeletonUtils::CalcSkinningMatrices(
		model, skinned.nodeGlobalTransforms, skinned.boneTransforms);
}