#pragma once
#include <d3d11.h>
#include <wrl.h>
#include <string>

class GpuResourceUtils
{
public:
    // 頂点シェーダー読み込み
    static HRESULT LoadVertexShader(
        ID3D11Device* device,
        const std::string& filename,
        const D3D11_INPUT_ELEMENT_DESC inputElementDescs[],
        UINT inputElementCount,
        Microsoft::WRL::ComPtr<ID3D11InputLayout>& inputLayout,
        Microsoft::WRL::ComPtr<ID3D11VertexShader>& vertexShader
    );

    // ピクセルシェーダー読み込み
    static HRESULT LoadPixelShader(
        ID3D11Device* device,
        const std::string& filename,
        Microsoft::WRL::ComPtr<ID3D11PixelShader>& pixelShader
    );

    // ファイルパスからテクスチャ読み込み
    static HRESULT LoadTexture(
        ID3D11Device* device,
        const std::string& filename,
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& shaderResourceView,
        D3D11_TEXTURE2D_DESC* texture2dDesc = nullptr
    );

    // 埋め込みテクスチャ読み込み
    static HRESULT LoadTexture(
        ID3D11Device* device,
        const unsigned char* data,
        size_t dataSize,
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& shaderResourceView
    );

    // DDSテクスチャ読み込み
    static HRESULT LoadDDSTexture(
        ID3D11Device* device,
        const std::string& filename,
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& shaderResourceView,
        bool* isCubemap = nullptr
    );

    // ダミーテクスチャ生成
    static HRESULT CreateDummyTexture(
        ID3D11Device* device,
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& shaderResourceView,
        DWORD value,
        UINT dimension
    );

    // 定数バッファ生成
    static HRESULT CreateConstantBuffer(
        ID3D11Device* device,
        UINT byteWidth,
        Microsoft::WRL::ComPtr<ID3D11Buffer>& constantBuffer
    );
};