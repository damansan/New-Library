#pragma once
#include <d3d11.h>
#include <DirectXMath.h>
#include "Resource/ResourceManager.h"
#include "Core/Camera.h"
#include "Editor/EditorContext.h"

// Scene系に渡すコンテキスト
// 必要なものが増えたらここに追加する
struct SceneContext
{
    ID3D11Device* device = nullptr;               // DX11デバイス
    ID3D11DeviceContext* deviceContext = nullptr;
    ResourceManager* resourceManager = nullptr;   // リソース管理

    ID3D11RenderTargetView* renderTargetView = nullptr;
    ID3D11DepthStencilView* depthStencilView = nullptr;
    
    Camera* camera = nullptr;
    DirectX::XMFLOAT4X4 view;
    DirectX::XMFLOAT4X4 projection;

    UINT screenWidth = 0;
    UINT screenHeight = 0;

    CameraMode cameraMode = CameraMode::Debug;
    EditorContext* editorContext = nullptr;
};