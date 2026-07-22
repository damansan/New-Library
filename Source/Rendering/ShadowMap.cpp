#include "ShadowMap.h"
#include "Utility/misc.h"

// 初期化
void ShadowMap::Initialize(ID3D11Device* device)
{
    // 深度専用テクスチャ配列の設定
    D3D11_TEXTURE2D_DESC texDesc{};
    texDesc.Width = ShadowMapSize;
    texDesc.Height = ShadowMapSize;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = CascadeCount; // カスケード数ぶんのスライスを持つ配列テクスチャにする
    // DSVとSRVの両方を作るため、型なしフォーマットで確保する
    texDesc.Format = DXGI_FORMAT_R32_TYPELESS;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

    HRESULT hr = device->CreateTexture2D(&texDesc, nullptr, texture.GetAddressOf());
    _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

    // カスケードごとのDSV(該当スライス1枚だけを描画対象にするビュー)
    for (int i = 0; i < CascadeCount; i++)
    {
        D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
        dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
        dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
        dsvDesc.Texture2DArray.MipSlice = 0;
        dsvDesc.Texture2DArray.FirstArraySlice = i;
        dsvDesc.Texture2DArray.ArraySize = 1;
        hr = device->CreateDepthStencilView(texture.Get(), &dsvDesc, dsvs[i].GetAddressOf());
        _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
    }

    // 全カスケードをまとめて読むSRV(Texture2DArray、1つだけ生成すればよい)
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
    srvDesc.Texture2DArray.MostDetailedMip = 0;
    srvDesc.Texture2DArray.MipLevels = 1;
    srvDesc.Texture2DArray.FirstArraySlice = 0;
    srvDesc.Texture2DArray.ArraySize = CascadeCount;
    hr = device->CreateShaderResourceView(texture.Get(), &srvDesc, srv.GetAddressOf());
    _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

    srvRaw = srv.Get();
}

// カスケードごとのライトビュープロジェクション行列を算出
void ShadowMap::UpdateMatrices(
    const DirectX::XMFLOAT3& cameraPosition,
    const DirectX::XMFLOAT3& cameraFront,
    const DirectX::XMFLOAT3& cameraRight,
    const DirectX::XMFLOAT3& cameraUp,
    float fovY,
    float aspect,
    const DirectX::XMFLOAT3& lightDirection)
{
    using namespace DirectX;

    XMVECTOR CameraPosition = XMLoadFloat3(&cameraPosition);
    XMVECTOR CameraFront = XMLoadFloat3(&cameraFront);
    XMVECTOR CameraRight = XMLoadFloat3(&cameraRight);
    XMVECTOR CameraUp = XMLoadFloat3(&cameraUp);

    // ライトの位置から見たビュー行列
    XMVECTOR LightDirection = XMVector3Normalize(XMLoadFloat3(&lightDirection));
    XMVECTOR LightPosition = XMVectorScale(LightDirection, -250.0f);
    XMMATRIX LightView = XMMatrixLookAtLH(
        LightPosition,
        XMVectorZero(),
        XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));

    // シャドウマップに描画したい範囲の射影行列
    XMMATRIX LightProjection = XMMatrixOrthographicLH(10000.0f, 10000.0f, 0.1f, 1000.0f);

    // ライトビュープロジェクション行列
    XMMATRIX LVP = LightView * LightProjection;

    for (int cascadeIndex = 0; cascadeIndex < CascadeCount; cascadeIndex++)
    {
        float nearDepth = SplitDistances[cascadeIndex + 0];
        float farDepth = SplitDistances[cascadeIndex + 1];

        // エリアを内包する視錐台の8頂点を算出
        float nearY = tanf(fovY * 0.5f) * nearDepth;
        float nearX = nearY * aspect;
        float farY = tanf(fovY * 0.5f) * farDepth;
        float farX = farY * aspect;

        XMVECTOR NearPosition = XMVectorAdd(CameraPosition, XMVectorScale(CameraFront, nearDepth));
        XMVECTOR FarPosition = XMVectorAdd(CameraPosition, XMVectorScale(CameraFront, farDepth));

        XMVECTOR vertex[8] =
        {
            // 近平面：右上・左上・右下・左下
            XMVectorAdd(NearPosition, XMVectorAdd(XMVectorScale(CameraUp,  nearY), XMVectorScale(CameraRight,  nearX))),
            XMVectorAdd(NearPosition, XMVectorAdd(XMVectorScale(CameraUp,  nearY), XMVectorScale(CameraRight, -nearX))),
            XMVectorAdd(NearPosition, XMVectorAdd(XMVectorScale(CameraUp, -nearY), XMVectorScale(CameraRight,  nearX))),
            XMVectorAdd(NearPosition, XMVectorAdd(XMVectorScale(CameraUp, -nearY), XMVectorScale(CameraRight, -nearX))),
            // 遠平面：右上・左上・右下・左下
            XMVectorAdd(FarPosition,  XMVectorAdd(XMVectorScale(CameraUp,  farY),  XMVectorScale(CameraRight,  farX))),
            XMVectorAdd(FarPosition,  XMVectorAdd(XMVectorScale(CameraUp,  farY),  XMVectorScale(CameraRight, -farX))),
            XMVectorAdd(FarPosition,  XMVectorAdd(XMVectorScale(CameraUp, -farY),  XMVectorScale(CameraRight,  farX))),
            XMVectorAdd(FarPosition,  XMVectorAdd(XMVectorScale(CameraUp, -farY),  XMVectorScale(CameraRight, -farX))),
        };

        // 8頂点をライトビュープロジェクション空間に変換し、最小値・最大値を算出
        XMFLOAT2 vertexMin(FLT_MAX, FLT_MAX);
        XMFLOAT2 vertexMax(-FLT_MAX, -FLT_MAX);
        for (auto& v : vertex)
        {
            XMFLOAT3 p;
            XMStoreFloat3(&p, XMVector3TransformCoord(v, LVP));
            vertexMin.x = (std::min)(p.x, vertexMin.x);
            vertexMin.y = (std::min)(p.y, vertexMin.y);
            vertexMax.x = (std::max)(p.x, vertexMax.x);
            vertexMax.y = (std::max)(p.y, vertexMax.y);
        }

        // 最小値・最大値からクロップ行列を算出
        float xScale = 2.0f / (vertexMax.x - vertexMin.x);
        float yScale = 2.0f / (vertexMax.y - vertexMin.y);
        float xOffset = -0.5f * (vertexMax.x + vertexMin.x) * xScale;
        float yOffset = -0.5f * (vertexMax.y + vertexMin.y) * yScale;

        XMMATRIX CropMatrix = XMMatrixIdentity();
        XMFLOAT4X4 crop;
        XMStoreFloat4x4(&crop, CropMatrix);
        crop._11 = xScale;
        crop._22 = yScale;
        crop._41 = xOffset;
        crop._42 = yOffset;
        CropMatrix = XMLoadFloat4x4(&crop);

        // ライトビュープロジェクション行列にクロップ行列を乗算して最終行列を算出
        XMStoreFloat4x4(&lightViewProjections[cascadeIndex], LVP * CropMatrix);
    }
}
