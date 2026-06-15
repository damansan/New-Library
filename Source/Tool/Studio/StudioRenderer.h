#pragma once
#include <d3d11.h>
#include <wrl.h>
#include <memory>
#include <vector>
#include <DirectXMath.h>

#include "Resource/Model.h"

class StudioRenderer
{
public:
    void Initialize(ID3D11Device* device);

    // IBLテクスチャセット（SkyBoxから借りる）
    void SetIBLTextures(
        ID3D11ShaderResourceView* diffuseIem,
        ID3D11ShaderResourceView* specularPmrem,
        ID3D11ShaderResourceView* lutGgx);

    // モデル描画
    void DrawModel(
        ID3D11DeviceContext* dc,
        const Model& model,
        const std::vector<std::vector<DirectX::XMFLOAT4X4>>& boneTransforms,
        const DirectX::XMFLOAT4X4& world,
        const DirectX::XMFLOAT4X4& view,
        const DirectX::XMFLOAT4X4& projection,
        const DirectX::XMFLOAT3& cameraPosition
    );

private:
    void UpdateCbScene(
        ID3D11DeviceContext* dc,
        const DirectX::XMFLOAT4X4& view,
        const DirectX::XMFLOAT4X4& projection,
        const DirectX::XMFLOAT3& cameraPosition);

    // シェーダー
    Microsoft::WRL::ComPtr<ID3D11VertexShader> staticVS;
    Microsoft::WRL::ComPtr<ID3D11VertexShader> skinnedVS;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>  forwardPS;
    Microsoft::WRL::ComPtr<ID3D11InputLayout>  inputLayout;

    // 定数バッファ
    Microsoft::WRL::ComPtr<ID3D11Buffer> cbScene;
    Microsoft::WRL::ComPtr<ID3D11Buffer> cbObject;
    Microsoft::WRL::ComPtr<ID3D11Buffer> cbSkinning;

    // ステート
    Microsoft::WRL::ComPtr<ID3D11SamplerState>      samplerState;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState>   rasterizerState;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depthStencilState;
    Microsoft::WRL::ComPtr<ID3D11BlendState>        blendState;

    // ダミーテクスチャ
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> dummyWhite;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> dummyNormal;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> dummyBlack;

    // IBL（外から借りるので生ポインタ）
    ID3D11ShaderResourceView* iblDiffuseIem = nullptr;
    ID3D11ShaderResourceView* iblSpecularPmrem = nullptr;
    ID3D11ShaderResourceView* iblLutGgx = nullptr;

    // 定数バッファ構造体
    struct CbScene
    {
        DirectX::XMFLOAT4X4 view;
        DirectX::XMFLOAT4X4 projection;
        DirectX::XMFLOAT4   cameraPosition;
    };

    struct CbObject
    {
        DirectX::XMFLOAT4X4 world;
    };

    static constexpr int MAX_BONES = 256;
    struct CbSkinning
    {
        DirectX::XMFLOAT4X4 boneTransforms[MAX_BONES];
    };
};