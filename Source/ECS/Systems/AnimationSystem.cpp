#include "AnimationSystem.h"
#include "tracy/Tracy.hpp"

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
				skinned.boneTransforms,
				skinned.nodeGlobalTransforms);
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
				skinned.boneTransforms,
				skinned.nodeGlobalTransforms);
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
	std::vector<std::vector<DirectX::XMFLOAT4X4>>& boneTransforms,
	std::vector<DirectX::XMFLOAT4X4>& nodeGlobalTransforms)
{
	auto& nodes = model.GetNodes();
	const auto& meshes = model.GetMeshes();

	// ノードのグローバル行列を計算
	nodeGlobalTransforms.resize(nodes.size());
	auto& globalTransforms = nodeGlobalTransforms;

	// ルートから再帰的にグローバル行列を計算
	std::function<void(int, const DirectX::XMFLOAT4X4&)> calcGlobal =
		[&](int nodeIndex, const DirectX::XMFLOAT4X4& parentTransform)
		{
			auto& node = nodes[nodeIndex];

			// キーフレームから補間してローカル行列を作る
			DirectX::XMFLOAT3 position = node.position;
			DirectX::XMFLOAT4 rotation = node.rotation;
			DirectX::XMFLOAT3 scale = node.scale;

			if (nodeIndex < (int)animation.nodeAnims.size())
			{
				const auto& nodeAnim = animation.nodeAnims[nodeIndex];

				if (!nodeAnim.positionKeyframes.empty())
				{
					position = InterpolatePosition(
						nodeAnim.positionKeyframes, time);
				}

				if (!nodeAnim.rotationKeyframes.empty())
				{
					rotation = InterpolateRotation(
						nodeAnim.rotationKeyframes, time);
				}

				if (!nodeAnim.scaleKeyframes.empty())
				{
					scale = InterpolateScale(
						nodeAnim.scaleKeyframes, time);
				}
			}

			// ローカル行列を計算
			DirectX::XMMATRIX S = DirectX::XMMatrixScaling(
				scale.x, scale.y, scale.z);
			DirectX::XMMATRIX R = DirectX::XMMatrixRotationQuaternion(
				DirectX::XMVectorSet(
					rotation.x, rotation.y,
					rotation.z, rotation.w));
			DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(
				position.x, position.y, position.z);

			DirectX::XMMATRIX local = S * R * T;
			DirectX::XMMATRIX parent = DirectX::XMLoadFloat4x4(&parentTransform);
			DirectX::XMMATRIX global = local * parent;

			DirectX::XMStoreFloat4x4(&globalTransforms[nodeIndex], global);


			// 子ノードを再帰処理
			for (int child : node.childIndices)
			{
				calcGlobal(child, globalTransforms[nodeIndex]);
			}
		};

	// ルートノードから計算開始
	DirectX::XMFLOAT4X4 identity =
	{ 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };

	for (int i = 0; i < (int)nodes.size(); i++)
	{
		if (nodes[i].parentIndex < 0)
		{
			calcGlobal(i, identity);
		}
	}

	if (meshes.empty()) return;

	boneTransforms.resize(meshes.size());

	for (int m = 0; m < (int)meshes.size(); m++)
	{
		const auto& mesh = meshes[m];
		boneTransforms[m].resize(mesh.bones.size());

		for (int i = 0; i < (int)mesh.bones.size(); i++)
		{
			const auto& bone = mesh.bones[i];
			int nodeIdx = bone.nodeIndex;
			if (nodeIdx < 0 || nodeIdx >= (int)globalTransforms.size()) continue;

			DirectX::XMMATRIX offset = DirectX::XMLoadFloat4x4(&bone.offsetTransform);
			DirectX::XMMATRIX global = DirectX::XMLoadFloat4x4(&globalTransforms[nodeIdx]);

			DirectX::XMStoreFloat4x4(&boneTransforms[m][i], offset * global);
		}
	}
}
