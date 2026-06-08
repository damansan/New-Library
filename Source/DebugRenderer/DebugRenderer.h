#pragma once
#include <d3d11.h>
#include <wrl.h>
#include <DirectXMath.h>
#include <vector>
#include <functional>

#include "Resource/Model.h"

class DebugRenderer
{
public:
    DebugRenderer() = default;
    ~DebugRenderer() = default;

    DebugRenderer(const DebugRenderer&) = delete;
    DebugRenderer& operator=(const DebugRenderer&) = delete;

    // 初期化
    void Initialize(ID3D11Device* device);

    // ライン追加
    void AddLine(
        const DirectX::XMFLOAT3& start,
        const DirectX::XMFLOAT3& end,
        const DirectX::XMFLOAT4& color = { 1,1,0,1 }
    );

    // ボーン描画
    void AddBones(
        const std::vector<Model::Node>& nodes,
        const DirectX::XMMATRIX& worldMatrix,
        const DirectX::XMFLOAT4& color = { 1,1,0,1 }
    );

    // 描画
    void Render(
        ID3D11DeviceContext* dc,
        const DirectX::XMFLOAT4X4& view,
        const DirectX::XMFLOAT4X4& projection
    );

    // フレーム先頭でクリア
    void Clear();

private:
    struct LineVertex
    {
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT4 color;
    };

    struct CbDebug
    {
        DirectX::XMFLOAT4X4 viewProjection;
    };

    Microsoft::WRL::ComPtr<ID3D11Buffer>            vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer>            constantBuffer;
    Microsoft::WRL::ComPtr<ID3D11VertexShader>      vertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>       pixelShader;
    Microsoft::WRL::ComPtr<ID3D11InputLayout>       inputLayout;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState>   rasterizerState;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depthStencilState;

    std::vector<LineVertex> vertices;

    static constexpr UINT MAX_LINES = 4096;
};