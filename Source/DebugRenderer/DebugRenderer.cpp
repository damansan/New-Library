#include "DebugRenderer.h"
#include "Resource/GpuResourceUtils.h"
#include "Utility/misc.h"

void DebugRenderer::Initialize(ID3D11Device* device)
{
    // 入力レイアウト定義
    D3D11_INPUT_ELEMENT_DESC inputElementDescs[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    // 頂点シェーダー読み込み
    GpuResourceUtils::LoadVertexShader(
        device,
        "Data/Shader/Debug_VS.cso",
        inputElementDescs,
        ARRAYSIZE(inputElementDescs),
        inputLayout,
        vertexShader
    );

    // ピクセルシェーダー読み込み
    GpuResourceUtils::LoadPixelShader(
        device,
        "Data/Shader/Debug_PS.cso",
        pixelShader
    );

    // 動的頂点バッファ生成（MAP/UNMAPで毎フレーム更新）
    D3D11_BUFFER_DESC vbDesc{};
    vbDesc.ByteWidth = sizeof(LineVertex) * MAX_LINES * 2; // 1ラインにつき2頂点
    vbDesc.Usage = D3D11_USAGE_DYNAMIC;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    HRESULT hr = device->CreateBuffer(&vbDesc, nullptr, vertexBuffer.GetAddressOf());
    _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

    // 定数バッファ生成
    GpuResourceUtils::CreateConstantBuffer(
        device,
        sizeof(CbDebug),
        constantBuffer
    );

    // ラスタライザーステート（ワイヤーフレーム、カリングなし）
    D3D11_RASTERIZER_DESC rasterizerDesc{};
    rasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;
    rasterizerDesc.CullMode = D3D11_CULL_NONE;
    rasterizerDesc.DepthClipEnable = TRUE;
    hr = device->CreateRasterizerState(&rasterizerDesc, rasterizerState.GetAddressOf());
    _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

    // 深度ステンシルステート（深度テストあり、書き込みなし）
    D3D11_DEPTH_STENCIL_DESC depthStencilDesc{};
    depthStencilDesc.DepthEnable = TRUE;
    depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO; // 深度書き込みなし
    depthStencilDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;     // 深度テスト常にパス
    hr = device->CreateDepthStencilState(&depthStencilDesc, depthStencilState.GetAddressOf());
    _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

    // 頂点リスト確保
    vertices.reserve(MAX_LINES * 2);
}

void DebugRenderer::AddLine(
    const DirectX::XMFLOAT3& start,
    const DirectX::XMFLOAT3& end,
    const DirectX::XMFLOAT4& color)
{
    // 最大数チェック
    if (vertices.size() >= MAX_LINES * 2) return;

    vertices.push_back({ start, color });
    vertices.push_back({ end,   color });
}

void DebugRenderer::AddBones(
    const std::vector<Model::Node>& nodes,
    const DirectX::XMMATRIX& worldMatrix,
    const DirectX::XMFLOAT4& color)
{
    for (int i = 0; i < (int)nodes.size(); i++)
    {
        const auto& node = nodes[i];
        if (node.parentIndex < 0) continue;

        const auto& parent = nodes[node.parentIndex];

        DirectX::XMFLOAT3 childPos = {
            node.globalTransform._41,
            node.globalTransform._42,
            node.globalTransform._43
        };
        DirectX::XMFLOAT3 parentPos = {
            parent.globalTransform._41,
            parent.globalTransform._42,
            parent.globalTransform._43
        };

        DirectX::XMStoreFloat3(&childPos,
            DirectX::XMVector3TransformCoord(
                DirectX::XMLoadFloat3(&childPos), worldMatrix));
        DirectX::XMStoreFloat3(&parentPos,
            DirectX::XMVector3TransformCoord(
                DirectX::XMLoadFloat3(&parentPos), worldMatrix));

        AddLine(parentPos, childPos, color);
    }
}

void DebugRenderer::Render(
    ID3D11DeviceContext* dc,
    const DirectX::XMFLOAT4X4& view,
    const DirectX::XMFLOAT4X4& projection)
{
    // 描画する頂点がなければスキップ
    if (vertices.empty()) return;

    // 頂点バッファをGPUに転送
    D3D11_MAPPED_SUBRESOURCE mapped{};
    HRESULT hr = dc->Map(vertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (SUCCEEDED(hr))
    {
        size_t copySize = sizeof(LineVertex) * vertices.size();
        memcpy(mapped.pData, vertices.data(), copySize);
        dc->Unmap(vertexBuffer.Get(), 0);
    }

    // ViewProjection行列を定数バッファに設定
    CbDebug cbData{};
    DirectX::XMMATRIX v = DirectX::XMLoadFloat4x4(&view);
    DirectX::XMMATRIX p = DirectX::XMLoadFloat4x4(&projection);
    DirectX::XMStoreFloat4x4(&cbData.viewProjection, v * p);
    dc->UpdateSubresource(constantBuffer.Get(), 0, nullptr, &cbData, 0, 0);

    // シェーダー設定
    dc->VSSetShader(vertexShader.Get(), nullptr, 0);
    dc->PSSetShader(pixelShader.Get(), nullptr, 0);
    dc->VSSetConstantBuffers(0, 1, constantBuffer.GetAddressOf());

    // 入力レイアウト設定
    dc->IASetInputLayout(inputLayout.Get());
    dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

    // 頂点バッファ設定
    UINT stride = sizeof(LineVertex);
    UINT offset = 0;
    dc->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &stride, &offset);

    // ステート設定
    dc->RSSetState(rasterizerState.Get());
    dc->OMSetDepthStencilState(depthStencilState.Get(), 0);

    // 描画
    dc->Draw(static_cast<UINT>(vertices.size()), 0);
}

void DebugRenderer::Clear()
{
    vertices.clear();
}