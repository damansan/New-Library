#pragma once
#include <d3d11.h>
#include <DirectXMath.h>
#include "entt.hpp"
#include "StudioContext.h"

class IStudioAttachment
{
public:
	virtual ~IStudioAttachment() = default;

    // çXêV
    virtual void Update(float elapsedTime, StudioContext& context) {}

    // 3Dï`âÊ
    virtual void OnRender(
        ID3D11DeviceContext* dc,
        const DirectX::XMFLOAT4X4& view,
        const DirectX::XMFLOAT4X4& projection,
        entt::registry& registry,
        StudioContext& context) 
    {}

    // ImGuiï`âÊ
    virtual void DrawGUI(
        entt::registry& registry,
        StudioContext& context)
    {}
};