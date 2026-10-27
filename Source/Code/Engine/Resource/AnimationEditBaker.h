#pragma once
#include <string>
#include <vector>
#include "Engine/Resource/Model.h"
#include "Engine/Data/AnimationEditData.h"

// アニメーションを編集する
class AnimationEditBaker
{
public:
	static std::vector<Model::Animation> Bake(
		const std::vector<Model::Animation>& rawAnimations,
		const std::vector<Model::Node>& nodes,
		const AnimationEditData& editData);

	// RootNodeの名前を探す
	static std::string FindDefaultRootNodeName(const Model& model);
private:
	// 名前からルートノードの番号を探す
	static int FindNodeIndex(const std::vector<Model::Node>& nodes, const std::string& name);

	// 0フレーム目のルート位置が、バインドポーズの位置になるようにずらす
	static void RemoveRootOffset(Model::NodeAnim& rootAnim, const Model::Node& rootNode);

	// ルートの移動量に倍率を掛ける
	static void ApplyRootMotionScale(Model::NodeAnim& rootAnim, const Model::Node& rootNode, float scaleXZ, float scaleY);

	// ルートをY軸回りに回す
	static void ApplyYawOffset(Model::NodeAnim& rootAnim, const Model::Node& rootNode, float yawDegrees);
};