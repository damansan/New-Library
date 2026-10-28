#include "AnimationEditBaker.h"
#include <algorithm>
#include <DirectXMath.h>

namespace
{
	// 指定時刻の値を補間で求める
	DirectX::XMFLOAT3 SampleVector(const std::vector<Model::VectorKeyframe>& keys, float time)
	{
		if (time <= keys.front().seconds) return keys.front().value;

		for (size_t i = 0; i + 1 < keys.size(); i++)
		{
			const auto& a = keys[i];
			const auto& b = keys[i + 1];
			if (time <= b.seconds)
			{
				const float span = b.seconds - a.seconds;
				const float t = (span > 0.0f) ? (time - a.seconds) / span : 0.0f;
				DirectX::XMFLOAT3 result;
				DirectX::XMStoreFloat3(&result, DirectX::XMVectorLerp(DirectX::XMLoadFloat3(&a.value), DirectX::XMLoadFloat3(&b.value), t));
				return result;
			}
		}
		return keys.back().value;
	}

	// 指定時刻の値を補間で求める
	DirectX::XMFLOAT4 SampleQuaternion(const std::vector<Model::QuaternionKeyframe>& keys, float time)
	{
		if (time <= keys.front().seconds) return keys.front().value;

		for (size_t i = 0; i + 1 < keys.size(); i++)
		{
			const auto& a = keys[i];
			const auto& b = keys[i + 1];
			if (time <= b.seconds)
			{
				const float span = b.seconds - a.seconds;
				const float t = (span > 0.0f) ? (time - a.seconds) / span : 0.0f;
				DirectX::XMFLOAT4 result;
				DirectX::XMStoreFloat4(&result, DirectX::XMQuaternionSlerp(DirectX::XMLoadFloat4(&a.value), DirectX::XMLoadFloat4(&b.value), t));
				return result;
			}
		}
		return keys.back().value;
	}

	// キー列を start〜end だけ残して、0秒始まりに詰める
	template<class Key, class Sampler>
	void TrimKeys(std::vector<Key>& keys, float start, float end, Sampler sample)
	{
		if (keys.empty()) return;

		// 1キーだけなら時間に関係なく同じ値なので、時刻だけ0にする
		if (keys.size() == 1)
		{
			keys[0].seconds = 0.0f;
			return;
		}

		std::vector<Key> result;

		// 切り口の値は補間で作る
		result.push_back({ 0.0f, sample(keys, start) });
		for (const auto& key : keys)
		{
			if (key.seconds > start && key.seconds < end)
			{
				result.push_back({ key.seconds - start, key.value });
			}
		}
		result.push_back({ end - start, sample(keys, end) });

		keys = std::move(result);
	}
}


std::vector<Model::Animation> AnimationEditBaker::Bake(
	const std::vector<Model::Animation>& rawAnimations,
	const std::vector<Model::Node>& nodes,
	const AnimationEditData& editData)
{
	const int rootIndex = FindNodeIndex(nodes, editData.rootNodeName);

	std::vector<Model::Animation> result;
	result.reserve(rawAnimations.size());

	for (const auto& raw : rawAnimations)
	{
		result.push_back(BakeClip(raw, nodes, rootIndex, editData.FindClip(raw.name)));
	}

	return result;
}

// 1クリップ分だけ焼く
Model::Animation AnimationEditBaker::BakeClip(
	const Model::Animation& rawAnimation,
	const std::vector<Model::Node>& nodes,
	int rootIndex,
	const AnimationEditClipData* clip)
{
	// 生データをコピーして、そこに加工を加える
	Model::Animation animation = rawAnimation;

	// カット
	float trimStart = 0.0f;
	if (clip)
	{
		const float length = animation.secondsLength;
		const float start = std::clamp(clip->trimStart, 0.0f, length);
		const float end = (clip->trimEnd < 0.0f) ? length : std::clamp(clip->trimEnd, start, length);

		// 範囲が有効で、実際に切る部分があるときだけ
		if (end - start > 0.0f && (start > 0.0f || end < length))
		{
			TrimAnimation(animation, start, end);
			trimStart = start;
		}
	}

	// ここから先はルートだけを加工する
	if (rootIndex < 0 || rootIndex >= (int)animation.nodeAnims.size()) return animation;
	Model::NodeAnim& rootAnim = animation.nodeAnims[rootIndex];
	const Model::Node& rootNode = nodes[rootIndex];

	// オフセット除去
	RemoveRootOffset(rootAnim, rootNode);

	if (!clip) return animation;

	// 区間の移動量
	if (!clip->sections.empty())
	{
		ApplySections(rootAnim, clip->sections, trimStart);
	}

	// 向きの補正
	if (clip->yawOffsetDegrees != 0.0f)
	{
		ApplyYawOffset(rootAnim, rootNode, clip->yawOffsetDegrees);
	}

	return animation;
}


// RootNodeの名前を探す
std::string AnimationEditBaker::FindDefaultRootNodeName(const Model& model)
{
	const auto& nodes = model.GetNodes();

	// どのノードがボーンかを調べる
	std::vector<bool> isBone(nodes.size(), false);
	for (const auto& mesh : model.GetMeshes())
	{
		for (const auto& bone : mesh.bones)
		{
			if (bone.nodeIndex >= 0 && bone.nodeIndex < (int)nodes.size())
			{
				isBone[bone.nodeIndex] = true;
			}
		}
	}

	// 親がいない、または親がボーンではないボーンが一番上
	for (int i = 0; i < (int)nodes.size(); i++)
	{
		if (!isBone[i]) continue;

		const int parent = nodes[i].parentIndex;
		if (parent < 0 || !isBone[parent])
		{
			return nodes[i].name;
		}
	}

	return "";
}

// 名前からルートノードの番号を探す
int AnimationEditBaker::FindNodeIndex(const std::vector<Model::Node>& nodes, const std::string& name)
{
	for (int i = 0; i < (int)nodes.size(); i++)
	{
		if (nodes[i].name == name) return i;
	}
	return -1;
}

// 指定範囲だけ残して、時間を0秒始まりに詰める
void AnimationEditBaker::TrimAnimation(Model::Animation& animation, float start, float end)
{
	for (auto& nodeAnim : animation.nodeAnims)
	{
		TrimKeys(nodeAnim.positionKeyframes, start, end, SampleVector);
		TrimKeys(nodeAnim.rotationKeyframes, start, end, SampleQuaternion);
		TrimKeys(nodeAnim.scaleKeyframes, start, end, SampleVector);
	}
	animation.secondsLength = end - start;
}

// 0フレーム目のルート位置が、バインドポーズの位置になるようにずらす
void AnimationEditBaker::RemoveRootOffset(Model::NodeAnim& rootAnim, const Model::Node& rootNode)
{
	if (rootAnim.positionKeyframes.empty()) return;

	// 0フレーム目がバインドポーズからどれだけずれているか
	const DirectX::XMFLOAT3& first = rootAnim.positionKeyframes.front().value;
	const DirectX::XMFLOAT3 offset =
	{
		first.x - rootNode.position.x,
		first.y - rootNode.position.y,
		first.z - rootNode.position.z
	};

	// 全キーから同じ量を引く
	for (auto& key : rootAnim.positionKeyframes)
	{
		key.value.x -= offset.x;
		key.value.y -= offset.y;
		key.value.z -= offset.z;
	}
}

// 区間ごとに、ルートの移動量に倍率を掛ける
void AnimationEditBaker::ApplySections(Model::NodeAnim& rootAnim, const std::vector<AnimationEditSection>& sections, float trimStart)
{
	auto& keys = rootAnim.positionKeyframes;
	if (keys.size() < 2) return;

	// 元の位置を控えておく
	std::vector<DirectX::XMFLOAT3> original(keys.size());
	for (size_t i = 0; i < keys.size(); i++) original[i] = keys[i].value;

	for (size_t i = 1; i < keys.size(); i++)
	{
		// このキー間の真ん中の時刻を、生データの時間に戻す
		const float mid = (keys[i - 1].seconds + keys[i].seconds) * 0.5f + trimStart;

		// その時刻を含む区間の倍率
		float scaleXZ = 1.0f;
		float scaleY = 1.0f;
		for (const auto& s : sections)
		{
			if (mid >= s.start && mid < s.end)
			{
				scaleXZ = s.scaleXZ;
				scaleY = s.scaleY;
				break;
			}
		}

		// 元の移動量に倍率を掛けて、ひとつ前のキーに積み上げる
		keys[i].value.x = keys[i - 1].value.x + (original[i].x - original[i - 1].x) * scaleXZ;
		keys[i].value.y = keys[i - 1].value.y + (original[i].y - original[i - 1].y) * scaleY;
		keys[i].value.z = keys[i - 1].value.z + (original[i].z - original[i - 1].z) * scaleXZ;
	}
}


// ルートをY軸回りに回す
void AnimationEditBaker::ApplyYawOffset(Model::NodeAnim& rootAnim, const Model::Node& rootNode, float yawDegrees)
{
	const DirectX::XMVECTOR yaw = DirectX::XMQuaternionRotationRollPitchYaw(0.0f, DirectX::XMConvertToRadians(yawDegrees), 0.0f);
	const DirectX::XMVECTOR pivot = XMLoadFloat3(&rootNode.position);

	// バインドポーズの位置を中心に回す
	for (auto& key : rootAnim.positionKeyframes)
	{
		DirectX::XMVECTOR p = DirectX::XMVectorSubtract(XMLoadFloat3(&key.value), pivot);
		p = DirectX::XMVector3Rotate(p, yaw);
		XMStoreFloat3(&key.value, DirectX::XMVectorAdd(p, pivot));
	}

	// 回転キーが無いクリップは、バインドポーズの回転を1キーとして足しておく
	if (rootAnim.rotationKeyframes.empty())
	{
		rootAnim.rotationKeyframes.push_back({ 0.0f, rootNode.rotation });
	}

	// 元の回転の「後に」Yawを掛ける
	for (auto& key : rootAnim.rotationKeyframes)
	{
		DirectX::XMVECTOR q = DirectX::XMLoadFloat4(&key.value);
		q = DirectX::XMQuaternionNormalize(DirectX::XMQuaternionMultiply(q, yaw));
		XMStoreFloat4(&key.value, q);
	}
}

