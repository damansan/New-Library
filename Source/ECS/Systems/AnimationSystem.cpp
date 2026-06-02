#include "AnimationSystem.h"
#include "ECS/Components/AnimatorComponent.h"
#include "ECS/Components/SkinnedMeshComponent.h"
#include "Resource/GLTFImporter.h"

// 更新
void AnimationSystem::Update(
	entt::registry& registry,
	float elapsedTime)
{
	auto view = registry.view<SkinnedMeshComponent, AnimatorComponent>();
	view.each(
		[&](auto entity,
			SkinnedMeshComponent& skinned,
			AnimatorComponent& animator)
		{
			if (!skinned.model) return;

			const auto& animations = skinned.model->GetAnimations();
			if (animations.empty()) return;

			// 現在のアニメーションIndexを取得
			// AnimatorComponent の currentAnimationName から検索
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

			const auto& anim = animations[animIndex];

			// 時間を進める
			if (animator.isPlaying)
			{
				animator.animationTime += elapsedTime * animator.playSpeed;
			}

			// ループ処理
			if (animator.animationTime > anim.secondsLength)
			{
				if (animator.loop)
				{
					animator.animationTime -= anim.secondsLength;
				}
				else
				{
					animator.animationTime = anim.secondsLength;
					animator.isPlaying = false;
					animator.isEnd = true;
				}
			}



			// ノードのSRTをキーフレームから更新
			const auto& nodeAnims = anim.nodeAnims;
			auto& nodes = skinned.model->GetNodes();

			for (int ni = 0; ni < (int)nodes.size(); ni++)
			{
				auto& node = nodes[ni];
				const auto& nodeAnim = nodeAnims[ni];

				// positionキーフレームがあれば補間
				if (!nodeAnim.positionKeyframes.empty())
				{
					node.position = SampleVector(
						nodeAnim.positionKeyframes, animator.animationTime);
				}

				// rotationキーフレームがあれば補間
				if (!nodeAnim.rotationKeyframes.empty())
				{
					node.rotation = SampleQuaternion(
						nodeAnim.rotationKeyframes, animator.animationTime);
				}

				// scaleキーフレームがあれば補間
				if (!nodeAnim.scaleKeyframes.empty())
				{
					node.scale = SampleVector(
						nodeAnim.scaleKeyframes, animator.animationTime);
				}
			}

			// globalTransformを再計算
			DirectX::XMFLOAT4X4 identity = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
			const auto& gltfScenes = skinned.model->GetNodes();

			// ルートノードを探して再帰計算
			for (int ni = 0; ni < (int)nodes.size(); ni++)
			{
				if (nodes[ni].parentIndex < 0)
				{
					GLTFImporter::CalcGlobalTransform(nodes, ni, identity);
				}
			}
		});
}


// Vectorキーフレームの線形補間
DirectX::XMFLOAT3 AnimationSystem::SampleVector(
	const std::vector<Model::VectorKeyframe>& keyframes,
	float time)
{
	if (keyframes.empty())
		return { 0, 0, 0 };

	// 最初のキーフレームより前
	if (time <= keyframes.front().seconds)
	{
		return keyframes.front().value;
	}

	// 最後のキーフレームより後
	if (time >= keyframes.back().seconds)
	{
		return keyframes.back().value;
	}

	// 前後のキーフレームを探す
	for (size_t i = 0; i + 1 < keyframes.size(); i++)
	{
		const auto& a = keyframes[i];
		const auto& b = keyframes[i + 1];

		if (time >= a.seconds && time < b.seconds)
		{
			// 補間率
			float t = (time - a.seconds) / (b.seconds - a.seconds);

			// 線形補間
			return {
				a.value.x + (b.value.x - a.value.x) * t,
				a.value.y + (b.value.y - a.value.y) * t,
				a.value.z + (b.value.z - a.value.z) * t
			};
		}
	}
	return keyframes.back().value;
}


// Quaternionキーフレームの球面線形補間
DirectX::XMFLOAT4 AnimationSystem::SampleQuaternion(const std::vector<Model::QuaternionKeyframe>& keyframes, float time)
{
	// wを正に統一するラムダ
	auto ensurePositiveW = [](DirectX::XMVECTOR q) -> DirectX::XMFLOAT4
		{
			q = DirectX::XMQuaternionNormalize(q);
			float w;
			DirectX::XMStoreFloat(&w, DirectX::XMVectorSplatW(q));
			if (w < 0.0f) q = DirectX::XMVectorNegate(q);
			DirectX::XMFLOAT4 result;
			DirectX::XMStoreFloat4(&result, q);
			return result;
		};

	if (keyframes.empty())
		return { 0, 0, 0, 1 };

	if (time <= keyframes.front().seconds)
	{
		DirectX::XMVECTOR q = DirectX::XMLoadFloat4(&keyframes.front().value);
		q = DirectX::XMQuaternionNormalize(q);
		DirectX::XMFLOAT4 result;
		DirectX::XMStoreFloat4(&result, q);
		return result;
		//return keyframes.front().value;
	}

	if (time >= keyframes.back().seconds)
	{
		DirectX::XMVECTOR q = DirectX::XMLoadFloat4(&keyframes.back().value);
		q = DirectX::XMQuaternionNormalize(q);
		DirectX::XMFLOAT4 result;
		DirectX::XMStoreFloat4(&result, q);
		return result;
		//return keyframes.back().value;
	}

	for (size_t i = 0; i + 1 < keyframes.size(); i++)
	{
		const auto& a = keyframes[i];
		const auto& b = keyframes[i + 1];

		if (time >= a.seconds && time < b.seconds)
		{
			float t = (time - a.seconds) / (b.seconds - a.seconds);

			// 球面線形補間（Slerp）
			DirectX::XMVECTOR qa = DirectX::XMLoadFloat4(&a.value);
			DirectX::XMVECTOR qb = DirectX::XMLoadFloat4(&b.value);

			// ドット積が負なら符号を反転して最短経路を取る
			float dot;
			DirectX::XMStoreFloat(&dot, DirectX::XMQuaternionDot(qa, qb));
			if (dot < 0.0f)
			{
				qb = DirectX::XMVectorNegate(qb);
			}

			DirectX::XMVECTOR q = DirectX::XMQuaternionSlerp(qa, qb, t);
			q = DirectX::XMQuaternionNormalize(q);
			DirectX::XMFLOAT4 result;
			DirectX::XMStoreFloat4(&result, q);
			return result;
		}
	}
	return keyframes.back().value;
}
