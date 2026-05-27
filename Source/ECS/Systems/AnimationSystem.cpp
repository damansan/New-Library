#include "AnimationSystem.h"
#include <algorithm>

// 更新
void AnimationSystem::Update(entt::registry& registry, float elapsedTime)
{
    auto view = registry.view<AnimatorComponent, SkinnedMeshComponent>();

    view.each([&](auto entity,
        AnimatorComponent& animator,
        SkinnedMeshComponent& skinnedMesh)
        {
            if (!skinnedMesh.model) return;
            if (!animator.isPlaying) return;

            const auto& animations = skinnedMesh.model->GetAnimations();
            if (animations.empty()) return;

            // アニメーション検索
            const Model::Animation* animation = nullptr;
            for (const auto& anim : animations)
            {
                if (anim.name == animator.currentAnimation)
                {
                    animation = &anim;
                    break;
                }
            }
            // 見つからなければ最初のアニメーション
            if (!animation) animation = &animations[0];

            // アニメーション時間更新
            animator.animationTime += elapsedTime * animator.playSpeed;

            // ループ処理
            if (animator.loop)
            {
                if (animator.animationTime > animation->secondsLength)
                {
                    animator.animationTime =
                        fmodf(animator.animationTime, animation->secondsLength);
                }
            }
            else
            {
                if (animator.animationTime >= animation->secondsLength)
                {
                    animator.animationTime = animation->secondsLength;
                    animator.isPlaying = false;
                    animator.isEnd = true;
                }
            }

            // ノードのローカルトランスフォーム更新
            auto& nodes = skinnedMesh.model->GetNodes();
            for (size_t i = 0; i < nodes.size(); i++)
            {
                if (i >= animation->nodeAnims.size()) break;

                const auto& nodeAnim = animation->nodeAnims[i];
                auto& node = nodes[i];

                // Position補間
                if (!nodeAnim.positionKeyframes.empty())
                {
                    node.position = InterpolatePosition(
                        nodeAnim.positionKeyframes, animator.animationTime);
                }
                // Rotation補間
                if (!nodeAnim.rotationKeyframes.empty())
                {
                    node.rotation = InterpolateRotation(
                        nodeAnim.rotationKeyframes, animator.animationTime);
                }
                // Scale補間
                if (!nodeAnim.scaleKeyframes.empty())
                {
                    node.scale = InterpolateScale(
                        nodeAnim.scaleKeyframes, animator.animationTime);
                }
            }

            // グローバルトランスフォーム更新
            DirectX::XMFLOAT4X4 identity =
            { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };

            for (size_t i = 0; i < nodes.size(); i++)
            {
                if (nodes[i].parentIndex == -1)
                {
                    UpdateGlobalTransform(nodes, static_cast<int>(i), identity);
                }
            }

            // ボーン行列更新
            const auto& meshes = skinnedMesh.model->GetMeshes();
            if (meshes.empty()) return;

            const auto& bones = meshes[0].bones;
            skinnedMesh.boneTransforms.resize(bones.size());

            for (size_t i = 0; i < bones.size(); i++)
            {
                const auto& bone = bones[i];
                int nodeIndex = bone.nodeIndex;

                if (nodeIndex < 0 ||
                    nodeIndex >= static_cast<int>(nodes.size())) continue;

                // ボーン行列 = offsetMatrix × globalTransform
                DirectX::XMMATRIX offset =
                    DirectX::XMLoadFloat4x4(&bone.offsetTransform);
                DirectX::XMMATRIX global =
                    DirectX::XMLoadFloat4x4(&nodes[nodeIndex].globalTransform);

                DirectX::XMStoreFloat4x4(
                    &skinnedMesh.boneTransforms[i],
                    DirectX::XMMatrixMultiply(offset, global)
                );
            }
        });
}

// キーフレーム補間(位置)
DirectX::XMFLOAT3 AnimationSystem::InterpolatePosition(const std::vector<Model::VectorKeyframe>& keyframes, float time)
{
    if (keyframes.size() == 1)
    {
        return keyframes[0].value;
    }

    // 前後キーフレーム検索
    for (size_t i = 0; i + 1 < keyframes.size(); i++)
    {
        if (time <= keyframes[i + 1].seconds)
        {
            float t = (time - keyframes[i].seconds) /
                (keyframes[i + 1].seconds - keyframes[i].seconds);

            DirectX::XMVECTOR a =
                DirectX::XMLoadFloat3(&keyframes[i].value);
            DirectX::XMVECTOR b =
                DirectX::XMLoadFloat3(&keyframes[i + 1].value);

            DirectX::XMFLOAT3 result;
            DirectX::XMStoreFloat3(&result,
                DirectX::XMVectorLerp(a, b, t));
            return result;
        }
    }
    return keyframes.back().value;
}

// キーフレーム補間(回転)
DirectX::XMFLOAT4 AnimationSystem::InterpolateRotation(const std::vector<Model::QuaternionKeyframe>& keyframes, float time)
{
    if (keyframes.size() == 1)
    {
        return keyframes[0].value;
    }

    for (size_t i = 0; i + 1 < keyframes.size(); i++)
    {
        if (time <= keyframes[i + 1].seconds)
        {
            float t = (time - keyframes[i].seconds) /
                (keyframes[i + 1].seconds - keyframes[i].seconds);

            DirectX::XMVECTOR a =
                DirectX::XMLoadFloat4(&keyframes[i].value);
            DirectX::XMVECTOR b =
                DirectX::XMLoadFloat4(&keyframes[i + 1].value);

            DirectX::XMFLOAT4 result;
            DirectX::XMStoreFloat4(&result,
                DirectX::XMQuaternionSlerp(a, b, t));
            return result;
        }
    }
    return keyframes.back().value;
}

// キーフレーム補間(サイズ)
DirectX::XMFLOAT3 AnimationSystem::InterpolateScale(const std::vector<Model::VectorKeyframe>& keyframes, float time)
{
    if (keyframes.size() == 1)
    {
        return keyframes[0].value;
    }

    for (size_t i = 0; i + 1 < keyframes.size(); i++)
    {
        if (time <= keyframes[i + 1].seconds)
        {
            float t = (time - keyframes[i].seconds) /
                (keyframes[i + 1].seconds - keyframes[i].seconds);

            DirectX::XMVECTOR a =
                DirectX::XMLoadFloat3(&keyframes[i].value);
            DirectX::XMVECTOR b =
                DirectX::XMLoadFloat3(&keyframes[i + 1].value);

            DirectX::XMFLOAT3 result;
            DirectX::XMStoreFloat3(&result,
                DirectX::XMVectorLerp(a, b, t));
            return result;
        }
    }
    return keyframes.back().value;
}

// グローバルトランスフォーム更新
void AnimationSystem::UpdateGlobalTransform(std::vector<Model::Node>& nodes, int nodeIndex, const DirectX::XMFLOAT4X4& parentTransform)
{
    auto& node = nodes[nodeIndex];

    DirectX::XMMATRIX S = DirectX::XMMatrixScaling(
        node.scale.x, node.scale.y, node.scale.z);
    DirectX::XMMATRIX R = DirectX::XMMatrixRotationQuaternion(
        DirectX::XMVectorSet(
            node.rotation.x, node.rotation.y,
            node.rotation.z, node.rotation.w));
    DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(
        node.position.x, node.position.y, node.position.z);

    DirectX::XMMATRIX local = S * R * T;
    DirectX::XMMATRIX parent =
        DirectX::XMLoadFloat4x4(&parentTransform);
    DirectX::XMMATRIX global = local * parent;

    DirectX::XMStoreFloat4x4(&node.globalTransform, global);

    for (int childIndex : node.childIndices)
    {
        UpdateGlobalTransform(nodes, childIndex, node.globalTransform);
    }
}
