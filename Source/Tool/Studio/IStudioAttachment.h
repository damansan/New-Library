#pragma once
#include <d3d11.h>
#include <DirectXMath.h>
#include "StudioContext.h"

class IStudioAttachment
{
public:
	virtual ~IStudioAttachment() = default;

    // çXêV
    virtual void Update(float elapsedTime, StudioContext& context) {}

    // 3Dï`âÊÅiStudioWindowÇÃRTVÇ…ï`âÊÇ≥ÇÍÇÈÅj
    virtual void OnRender(
        ID3D11DeviceContext* dc,
        const DirectX::XMFLOAT4X4& view,
        const DirectX::XMFLOAT4X4& projection,
        StudioContext& context) {
    }

    // ImGuiï`âÊ
    virtual void DrawGUI(StudioContext& context) {}
};