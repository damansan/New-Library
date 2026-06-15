#include "StudioRenderer.h"
#include "Resource/GpuResourceUtils.h"

void StudioRenderer::Initialize(ID3D11Device* device)
{
    const auto& inputElementDescs = Model::InputElementDescs;

    // 静的メッシュ用VS
    GpuResourceUtils::LoadVertexShader(
        device,
        "Data/Shader/Static_VS.cso",
        inputElementDescs.data(),
        static_cast<UINT>(inputElementDescs.size()),
        inputLayout,
        staticVS
    );

    // スキニング用VS
    Microsoft::WRL::ComPtr<ID3D11InputLayout> dummyLayout;
    GpuResourceUtils::LoadVertexShader(
        device,
        "Data/Shader/Skinning_VS.cso",
        inputElementDescs.data(),
        static_cast<UINT>(inputElementDescs.size()),
        dummyLayout,
        skinnedVS
    );

    // フォワードPBR PS
    GpuResourceUtils::LoadPixelShader(
        device,
        "Data/Shader/PBR_PS.cso",
        forwardPS
    );

    // 定数バッファ
    GpuResourceUtils::CreateConstantBuffer(device, sizeof(CbScene), cbScene);
    GpuResourceUtils::CreateConstantBuffer(device, sizeof(CbObject), cbObject);
    GpuResourceUtils::CreateConstantBuffer(device, sizeof(CbSkinning), cbSkinning);

    // サンプラー
    D3D11_SAMPLER_DESC samplerDesc{};
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
    device->CreateSamplerState(&samplerDesc, samplerState.GetAddressOf());

    // ラスタライザー
    D3D11_RASTERIZER_DESC rasterizerDesc{};
    rasterizerDesc.FillMode = D3D11_FILL_SOLID;
    rasterizerDesc.CullMode = D3D11_CULL_BACK;
    rasterizerDesc.FrontCounterClockwise = FALSE;
    device->CreateRasterizerState(&rasterizerDesc, rasterizerState.GetAddressOf());

    // 深度ステンシル
    D3D11_DEPTH_STENCIL_DESC depthDesc{};
    depthDesc.DepthEnable = TRUE;
    depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depthDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
    device->CreateDepthStencilState(&depthDesc, depthStencilState.GetAddressOf());

    // ブレンドステート
    D3D11_BLEND_DESC blendDesc{};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    device->CreateBlendState(&blendDesc, blendState.GetAddressOf());

    // ダミーテクスチャ
    GpuResourceUtils::CreateDummyTexture(device, dummyWhite, 0xFFFFFFFF, 1);
    GpuResourceUtils::CreateDummyTexture(device, dummyNormal, 0xFFFF8080, 1);
    GpuResourceUtils::CreateDummyTexture(device, dummyBlack, 0xFF000000, 1);
}

// IBLテクスチャセット
void StudioRenderer::SetIBLTextures(
    ID3D11ShaderResourceView* diffuseIem,
    ID3D11ShaderResourceView* specularPmrem, 
    ID3D11ShaderResourceView* lutGgx)
{
    iblDiffuseIem = diffuseIem;
    iblSpecularPmrem = specularPmrem;
    iblLutGgx = lutGgx;
}

void StudioRenderer::DrawModel(
    ID3D11DeviceContext* dc, 
    const Model& model,
    const std::vector<std::vector<DirectX::XMFLOAT4X4>>& boneTransforms, const DirectX::XMFLOAT4X4& world, const DirectX::XMFLOAT4X4& view, const DirectX::XMFLOAT4X4& projection, const DirectX::XMFLOAT3& cameraPosition)
{
}
