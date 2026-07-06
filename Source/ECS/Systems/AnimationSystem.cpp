#include "AnimationSystem.h"
#include "tracy/Tracy.hpp"
#include "Resource/SkeletonUtils.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/MovementComponent.h"

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
			const float oldTime = animator.animationTime;

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

			// ルートモーションの移動量を計算
			animator.rootMotionDelta = { 0.0f, 0.0f, 0.0f };
			int rootMotionNodeIndex = -1;
			if (animator.useRootMotion)
			{
				rootMotionNodeIndex = FindNodeIndex(*skinned.model, animator.rootMotionNodeName);
				if (rootMotionNodeIndex >= 0)
				{
					animator.rootMotionDelta = CalcRootMotionDelta(
						*skinned.model, animation, rootMotionNodeIndex,
						oldTime, animator.animationTime);
				}
			}

			// ボーン行列を計算
			CalcBoneTransforms(
				*skinned.model,
				animation, 
				animator.animationTime,
				skinned,
				rootMotionNodeIndex);

			if (rootMotionNodeIndex >= 0 && elapsedTime > 0.0f)
			{
				auto* transform = registry.try_get<TransformComponent>(entity);
				auto* movement = registry.try_get<MovementComponent>(entity);

				if (transform && movement)
				{
					// ローカルの移動量を、キャラクターの向きに応じてワールド空間に変換する
					DirectX::XMMATRIX rotationMatrix = DirectX::XMMatrixRotationRollPitchYaw(
						transform->rotation.x, transform->rotation.y, transform->rotation.z);

					DirectX::XMVECTOR localDelta = DirectX::XMLoadFloat3(&animator.rootMotionDelta);
					DirectX::XMVECTOR worldDelta = DirectX::XMVector3TransformNormal(localDelta, rotationMatrix);

					DirectX::XMFLOAT3 worldDeltaF;
					DirectX::XMStoreFloat3(&worldDeltaF, worldDelta);

					// 移動量(位置の差分)を速度に変換して上書きする
					// （PhysicsSystem側で velocity * elapsedTime として使われるため）
					movement->velocity.x = worldDeltaF.x / elapsedTime;
					movement->velocity.z = worldDeltaF.z / elapsedTime;

					if (animator.useRootMotionY)
					{
						movement->velocity.y = worldDeltaF.y / elapsedTime;
					}
				}
			}
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
	SkinnedMeshComponent& skinned,
	int rootMotionNodeIndex)
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
		
		if (i == rootMotionNodeIndex)
		{
			skinned.localPositions[i] = CalcNodeLocalPosition(
				animation, i, nodes[i], 0.0f);
		}
	}

	// 階層計算とスキニング行列化は共通ロジック(SkeletonUtils)に任せる
	SkeletonUtils::CalcGlobalTransforms(
		nodes, skinned.localPositions, skinned.localRotations, skinned.localScales,
		skinned.nodeGlobalTransforms);

	SkeletonUtils::CalcSkinningMatrices(
		model, skinned.nodeGlobalTransforms, skinned.boneTransforms);
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
	float newTime)
{
	const auto& node = model.GetNodes()[rootMotionNodeIndex];

	// ポーズ取得用のラムダ（毎回同じ引数を書かずに済むように）
	auto getPos = [&](float t)
		{
			return CalcNodeLocalPosition(animation, rootMotionNodeIndex, node, t);
		};

	DirectX::XMFLOAT3 delta{ 0.0f, 0.0f, 0.0f };

	if (oldTime > newTime)
	{
		// ループで終端→開始に巻き戻った場合
		// 「old→終端」の移動と「開始→new」の移動を合成する
		DirectX::XMFLOAT3 beginPos = getPos(0.0f);
		DirectX::XMFLOAT3 oldPos = getPos(oldTime);
		DirectX::XMFLOAT3 endPos = getPos(animation.secondsLength);
		DirectX::XMFLOAT3 newPos = getPos(newTime);

		delta.x = (endPos.x - oldPos.x) + (newPos.x - beginPos.x);
		delta.y = (endPos.y - oldPos.y) + (newPos.y - beginPos.y);
		delta.z = (endPos.z - oldPos.z) + (newPos.z - beginPos.z);
	}
	else
	{
		// 通常時：前フレームと今フレームの差分がそのまま移動量
		DirectX::XMFLOAT3 oldPos = getPos(oldTime);
		DirectX::XMFLOAT3 newPos = getPos(newTime);

		delta.x = newPos.x - oldPos.x;
		delta.y = newPos.y - oldPos.y;
		delta.z = newPos.z - oldPos.z;
	}

	return delta;
}
