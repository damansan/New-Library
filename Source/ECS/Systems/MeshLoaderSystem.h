#pragma once
#include <d3d11.h>

#include "entt.hpp"

#include "ECS/Components/MeshComponent.h"
#include "ECS/Components/StaticMeshComponent.h"
#include "ECS/Components/SkinnedMeshComponent.h"
#include "ECS/Components/AnimatorComponent.h"
#include "ECS/Components/Tags.h"

#include "Resource/ResourceManager.h"

class MeshLoaderSystem
{
public:
    // 更新(未ロードのMeshComponentを検出してロード）
    void Update(
        entt::registry& registry,
        ID3D11Device* device,
        ResourceManager& resourceManager
    );
};