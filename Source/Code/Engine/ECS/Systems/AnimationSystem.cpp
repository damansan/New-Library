#include "AnimationSystem.h"
#include "tracy/Tracy.hpp"
#include "Engine/Resource/SkeletonUtils.h"
#include "Engine/ECS/Components/TransformComponent.h"
#include "Engine/ECS/Components/MovementComponent.h"

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

			// 新クリップの時間を進める
			int animIndex = -1;
			for (int i = 0; i < (int)animations.size(); i++)
			{
				if (animations[i].name == animator.currentAnimation) { animIndex = i; break; }
			}
			if (animIndex < 0)
			{
				animIndex = 0;
				animator.currentAnimation = animations[0].name;
			}

			const auto& animation = animations[animIndex];
			
			// トリミングを秒数に変換
			if (!animator.trimResolved)
			{
				if (animator.trimEnd >= 0.0f)
				{
					animator.trimStart *= animation.secondsLength;
					animator.trimEnd *= animation.secondsLength;
					animator.animationTime = animator.trimStart;
				}
				animator.trimResolved = true;
			}

			const float effectiveEnd = (animator.trimEnd >= 0.0f) ? animator.trimEnd : animation.secondsLength;
			const float effectiveStart = (animator.trimEnd >= 0.0f) ? animator.trimStart : 0.0f;
			
			const float oldTime = animator.animationTime;

			animator.animationTime += elapsedTime * animator.playSpeed * animator.speedCurveMultiplier;
			// ループ処理
			if (animator.loop)
			{
				if (animator.animationTime > effectiveEnd)
				{
					animator.animationTime = effectiveStart + (animator.animationTime - effectiveEnd);
					animator.isEnd = false;
				}
			}
			else
			{
				if (animator.animationTime >= effectiveEnd)
				{
					animator.animationTime = effectiveEnd;
					animator.isPlaying = false;
					animator.isEnd = true;
				}
			}

			// 新クリップのルートモーション量とポーズを計算
			animator.rootMotionDelta = { 0.0f, 0.0f, 0.0f };
			int rootMotionNodeIndex = -1;
			if (!animator.rootMotionNodeName.empty())
			{
				rootMotionNodeIndex = FindNodeIndex(*skinned.model, animator.rootMotionNodeName);
				if (rootMotionNodeIndex >= 0)
				{
					animator.rootMotionDelta = CalcRootMotionDelta(
						*skinned.model, animation, rootMotionNodeIndex, oldTime, animator.animationTime,
						effectiveStart, effectiveEnd);
				}
			}

			// ボーン行列を計算
			CalcBoneTransforms(
				*skinned.model, animation, animator.animationTime,
				skinned.localPositions, skinned.localRotations, skinned.localScales,
				rootMotionNodeIndex);

			// 新クリップ由来のワールド速度
			auto* transform = registry.try_get<TransformComponent>(entity);
			auto* movement = registry.try_get<MovementComponent>(entity);

			DirectX::XMFLOAT3 newVelocity = { 0.0f, 0.0f, 0.0f };
			bool newHasVelocity = false;

			if (animator.useRootMotion && rootMotionNodeIndex >= 0 && elapsedTime > 0.0f && transform && movement)
			{
				DirectX::XMMATRIX rotationMatrix = DirectX::XMMatrixRotationRollPitchYaw(
					transform->rotation.x, transform->rotation.y, transform->rotation.z);
				DirectX::XMVECTOR localDelta = DirectX::XMLoadFloat3(&animator.rootMotionDelta);
				DirectX::XMVECTOR worldDelta = DirectX::XMVector3TransformNormal(localDelta, rotationMatrix);
				DirectX::XMFLOAT3 worldDeltaF;
				DirectX::XMStoreFloat3(&worldDeltaF, worldDelta);

				newVelocity.x = worldDeltaF.x / elapsedTime;
				newVelocity.z = worldDeltaF.z / elapsedTime;
				if (animator.useRootMotionY)
				{
					newVelocity.y = worldDeltaF.y / elapsedTime;
				}
				newHasVelocity = true;
			}

			// ブレンド中なら旧クリップも計算して混ぜる
			if (animator.isBlending)
			{
				int oldAnimIndex = -1;
				for (int i = 0; i < (int)animations.size(); i++)
				{
					if (animations[i].name == animator.blendAnimation) { oldAnimIndex = i; break; }
				}

				if (oldAnimIndex >= 0)
				{
					const auto& oldAnimation = animations[oldAnimIndex];
					const float oldOldTime = animator.blendAnimationTime;

					// トリミングされていた場合、そこを超えて進めない
					float oldEffectiveEnd = (animator.blendTrimEnd >= 0.0f) ? animator.blendTrimEnd : oldAnimation.secondsLength;

					animator.blendAnimationTime += elapsedTime * animator.playSpeed;
					if (animator.blendAnimationTime > oldEffectiveEnd)
					{
						animator.blendAnimationTime = oldEffectiveEnd;
					}

					int oldRootMotionNodeIndex = -1;
					DirectX::XMFLOAT3 oldRootMotionDelta = { 0.0f, 0.0f, 0.0f };
					if (!animator.rootMotionNodeName.empty())
					{
						oldRootMotionNodeIndex = FindNodeIndex(*skinned.model, animator.rootMotionNodeName);
						if (oldRootMotionNodeIndex >= 0)
						{
							oldRootMotionDelta = CalcRootMotionDelta(
								*skinned.model, oldAnimation, oldRootMotionNodeIndex, oldOldTime, animator.blendAnimationTime);
						}
					}

					std::vector<DirectX::XMFLOAT3> blendPositions;
					std::vector<DirectX::XMFLOAT4> blendRotations;
					std::vector<DirectX::XMFLOAT3> blendScales;
					CalcBoneTransforms(
						*skinned.model, oldAnimation, animator.blendAnimationTime,
						blendPositions, blendRotations, blendScales,
						oldRootMotionNodeIndex);

					// 進捗を進める
					animator.blendRate += (animator.blendTime > 0.0f) ? (elapsedTime / animator.blendTime) : 1.0f;
					float rate = std::clamp(animator.blendRate, 0.0f, 1.0f);

					// ボーン行列を混ぜる
					BlendPoses(
						blendPositions, blendRotations, blendScales,
						skinned.localPositions, skinned.localRotations, skinned.localScales,
						rate,
						skinned.localPositions, skinned.localRotations, skinned.localScales);

					// ルートモーションブレンド(旧新でルートモーション使用)
					if (animator.blendUseRootMotion && newHasVelocity && transform && movement)
					{
						DirectX::XMMATRIX rotationMatrix = DirectX::XMMatrixRotationRollPitchYaw(
							transform->rotation.x, transform->rotation.y, transform->rotation.z);
						DirectX::XMVECTOR localDelta = DirectX::XMLoadFloat3(&oldRootMotionDelta);
						DirectX::XMVECTOR worldDelta = DirectX::XMVector3TransformNormal(localDelta, rotationMatrix);
						DirectX::XMFLOAT3 worldDeltaF;
						DirectX::XMStoreFloat3(&worldDeltaF, worldDelta);

						DirectX::XMFLOAT3 oldVelocity = { worldDeltaF.x / elapsedTime, 0.0f, worldDeltaF.z / elapsedTime };
						if (animator.blendUseRootMotionY) oldVelocity.y = worldDeltaF.y / elapsedTime;

						movement->velocity.x = oldVelocity.x * (1.0f - rate) + newVelocity.x * rate;
						movement->velocity.z = oldVelocity.z * (1.0f - rate) + newVelocity.z * rate;
						if (animator.blendUseRootMotionY || animator.useRootMotionY)
							movement->velocity.y = oldVelocity.y * (1.0f - rate) + newVelocity.y * rate;
					}
					else if (newHasVelocity && movement)
					{
						// 片側だけルートモーション使用の場合は
						// 新クリップの速度を使用
						movement->velocity.x = newVelocity.x;
						movement->velocity.z = newVelocity.z;
						if (animator.useRootMotionY) movement->velocity.y = newVelocity.y;
					}

					if (animator.blendRate >= 1.0f)
					{
						animator.isBlending = false;
					}
				}
				else
				{
					// 旧クリップが見つからない場合はブレンドを諦める
					animator.isBlending = false;
					if (newHasVelocity && movement)
					{
						movement->velocity.x = newVelocity.x;
						movement->velocity.z = newVelocity.z;
						if (animator.useRootMotionY) movement->velocity.y = newVelocity.y;
					}
				}
			}
			else if (newHasVelocity && movement)
			{
				movement->velocity.x = newVelocity.x;
				movement->velocity.z = newVelocity.z;
				if (animator.useRootMotionY) movement->velocity.y = newVelocity.y;
			}

			SkeletonUtils::CalcGlobalTransforms(
				skinned.model->GetNodes(), skinned.localPositions, skinned.localRotations, skinned.localScales,
				skinned.nodeGlobalTransforms);

			SkeletonUtils::CalcSkinningMatrices(
				*skinned.model, skinned.nodeGlobalTransforms, skinned.boneTransforms);
		});


}

// 時間を進めずに、指定した時刻のポーズだけ計算する
void AnimationSystem::EvaluatePose(entt::registry& registry)
{
	ZoneScoped;

	auto view = registry.view<AnimatorComponent, SkinnedMeshComponent>();
	view.each(
		[&](auto entity, AnimatorComponent& animator, SkinnedMeshComponent& skinned)
		{
			if (!skinned.model) return;

			const auto& animations = skinned.model->GetAnimations();
			if (animations.empty()) return;

			int animIndex = -1;
			for (int i = 0; i < (int)animations.size(); i++)
			{
				if (animations[i].name == animator.currentAnimation) { animIndex = i; break; }
			}
			if (animIndex < 0) return;

			const auto& animation = animations[animIndex];

			CalcBoneTransforms(
				*skinned.model, animation, animator.animationTime,
				skinned.localPositions, skinned.localRotations, skinned.localScales);

			SkeletonUtils::CalcGlobalTransforms(
				skinned.model->GetNodes(), skinned.localPositions, skinned.localRotations, skinned.localScales,
				skinned.nodeGlobalTransforms);

			SkeletonUtils::CalcSkinningMatrices(
				*skinned.model, skinned.nodeGlobalTransforms, skinned.boneTransforms);
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
	std::vector<DirectX::XMFLOAT3>& outPositions,
	std::vector<DirectX::XMFLOAT4>& outRotations,
	std::vector<DirectX::XMFLOAT3>& outScales,
	int rootMotionNodeIndex) 
{
	auto& nodes = model.GetNodes();

	outPositions.resize(nodes.size());
	outRotations.resize(nodes.size());
	outScales.resize(nodes.size());

	for (int i = 0; i < (int)nodes.size(); i++)
	{
		outPositions[i] = nodes[i].position;
		outRotations[i] = nodes[i].rotation;
		outScales[i] = nodes[i].scale;

		// キーフレームがあれば補間して上書き
		if (i < (int)animation.nodeAnims.size())
		{
			const auto& nodeAnim = animation.nodeAnims[i];

			if (!nodeAnim.positionKeyframes.empty())
			{
				outPositions[i] = InterpolatePosition(nodeAnim.positionKeyframes, time);
			}
			if (!nodeAnim.rotationKeyframes.empty())
			{
				outRotations[i] = InterpolateRotation(nodeAnim.rotationKeyframes, time);
			}
			if (!nodeAnim.scaleKeyframes.empty())
			{
				outScales[i] = InterpolateScale(nodeAnim.scaleKeyframes, time);
			}
		}

		if (i == rootMotionNodeIndex)
		{
			outPositions[i] = CalcNodeLocalPosition(animation, i, nodes[i], 0.0f);
		}
	}
}

// 2つのポーズをrateで補間する
void AnimationSystem::BlendPoses(
	const std::vector<DirectX::XMFLOAT3>& posA,
	const std::vector<DirectX::XMFLOAT4>& rotA,
	const std::vector<DirectX::XMFLOAT3>& scaleA,
	const std::vector<DirectX::XMFLOAT3>& posB,
	const std::vector<DirectX::XMFLOAT4>& rotB,
	const std::vector<DirectX::XMFLOAT3>& scaleB,
	float rate,
	std::vector<DirectX::XMFLOAT3>& outPositions,
	std::vector<DirectX::XMFLOAT4>& outRotations,
	std::vector<DirectX::XMFLOAT3>& outScales)
{
	size_t count = posA.size();

	std::vector<DirectX::XMFLOAT3> tempPos(count);
	std::vector<DirectX::XMFLOAT4> tempRot(count);
	std::vector<DirectX::XMFLOAT3> tempScale(count);

	for (size_t i = 0; i < count; i++)
	{
		DirectX::XMStoreFloat3(&tempPos[i],
			DirectX::XMVectorLerp(DirectX::XMLoadFloat3(&posA[i]), DirectX::XMLoadFloat3(&posB[i]), rate));

		DirectX::XMStoreFloat4(&tempRot[i],
			DirectX::XMQuaternionSlerp(DirectX::XMLoadFloat4(&rotA[i]), DirectX::XMLoadFloat4(&rotB[i]), rate));

		DirectX::XMStoreFloat3(&tempScale[i],
			DirectX::XMVectorLerp(DirectX::XMLoadFloat3(&scaleA[i]), DirectX::XMLoadFloat3(&scaleB[i]), rate));
	}

	outPositions = std::move(tempPos);
	outRotations = std::move(tempRot);
	outScales = std::move(tempScale);
}

// 名前からルートノードインデックスを探す
int AnimationSystem::FindNodeIndex(const Model& model, const std::string& name)
{
	const auto& nodes = model.GetNodes();
	for (int i = 0; i < (int)nodes.size(); i++)
	{
		if (nodes[i].name == name) return i;
	}
	return -1;
}

// 指定時刻のローカル位置を求める
DirectX::XMFLOAT3 AnimationSystem::CalcNodeLocalPosition(
	const Model::Animation& animation,
	int nodeIndex,
	const Model::Node& node,
	float time)
{
	// アニメーションが無ければバインドポーズの位置を返す
	if (nodeIndex >= (int)animation.nodeAnims.size())
	{
		return node.position;
	}

	const auto& nodeAnim = animation.nodeAnims[nodeIndex];
	if (nodeAnim.positionKeyframes.empty())
	{
		return node.position;
	}

	return InterpolatePosition(nodeAnim.positionKeyframes, time);
}

// ルートモーションの移動量を計算する
DirectX::XMFLOAT3 AnimationSystem::CalcRootMotionDelta(
	const Model& model,
	const Model::Animation& animation,
	int rootMotionNodeIndex,
	float oldTime,
	float newTime,
	float rangeStart,
	float rangeEnd) 
{
	const auto& node = model.GetNodes()[rootMotionNodeIndex];

	// ポーズ取得用のラムダ
	auto getPos = [&](float t)
		{
			return CalcNodeLocalPosition(animation, rootMotionNodeIndex, node, t);
		};

	DirectX::XMFLOAT3 delta{ 0.0f, 0.0f, 0.0f };
	float actualRangeEnd = (rangeEnd >= 0.0f) ? rangeEnd : animation.secondsLength;

	if (oldTime > newTime)
	{
		DirectX::XMFLOAT3 beginPos = getPos(rangeStart);
		DirectX::XMFLOAT3 oldPos = getPos(oldTime);
		DirectX::XMFLOAT3 endPos = getPos(actualRangeEnd);
		DirectX::XMFLOAT3 newPos = getPos(newTime);

		delta.x = (endPos.x - oldPos.x) + (newPos.x - beginPos.x);
		delta.y = (endPos.y - oldPos.y) + (newPos.y - beginPos.y);
		delta.z = (endPos.z - oldPos.z) + (newPos.z - beginPos.z);
	}
	else
	{
		DirectX::XMFLOAT3 oldPos = getPos(oldTime);
		DirectX::XMFLOAT3 newPos = getPos(newTime);

		delta.x = newPos.x - oldPos.x;
		delta.y = newPos.y - oldPos.y;
		delta.z = newPos.z - oldPos.z;
	}

	return delta;
}
