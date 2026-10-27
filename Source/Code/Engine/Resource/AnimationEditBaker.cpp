#include "AnimationEditBaker.h"

std::vector<Model::Animation> AnimationEditBaker::Bake(
	const std::vector<Model::Animation>& rawAnimations,
	const std::vector<Model::Node>& nodes,
	const AnimationEditData& editData)
{
	// 生データをコピーして、コピーしたものに加工を加える
	std::vector<Model::Animation> result = rawAnimations;

	const int rootIndex = FindNodeIndex(nodes, editData.rootNodeName);
	if (rootIndex < 0) return result; // ルートが見つからなければ加工しない

	const Model::Node& rootNode = nodes[rootIndex];

	for (auto& animation : result)
	{
		if (rootIndex >= (int)animation.nodeAnims.size()) continue;
		Model::NodeAnim& rootAnim = animation.nodeAnims[rootIndex];

		// オフセット除去
		RemoveRootOffset(rootAnim, rootNode);

		const AnimationEditClipData* clip = editData.FindClip(animation.name);
		if (!clip) continue; // 設定が無いクリップはオフセット除去だけ

		// 移動量の倍率
		if (clip->rootMotionScaleXZ != 1.0f || clip->rootMotionScaleY != 1.0f)
		{
			ApplyRootMotionScale(rootAnim, rootNode, clip->rootMotionScaleXZ, clip->rootMotionScaleY);
		}

		// 向きの補正
		if (clip->yawOffsetDegrees != 0.0f)
		{
			ApplyYawOffset(rootAnim, rootNode, clip->yawOffsetDegrees);
		}
	}

	return result;
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

// ルートの移動量に倍率を掛ける
void AnimationEditBaker::ApplyRootMotionScale(Model::NodeAnim& rootAnim, const Model::Node& rootNode, float scaleXZ, float scaleY)
{
	const DirectX::XMFLOAT3& pivot = rootNode.position;

	// バインドポーズの位置を基準に伸び縮みさせる
	for (auto& key : rootAnim.positionKeyframes)
	{
		key.value.x = pivot.x + (key.value.x - pivot.x) * scaleXZ;
		key.value.y = pivot.y + (key.value.y - pivot.y) * scaleY;
		key.value.z = pivot.z + (key.value.z - pivot.z) * scaleXZ;
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

