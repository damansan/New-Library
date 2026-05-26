#include "GpuResourceUtils.h"
#include "Utility/misc.h"

#include <DDSTextureLoader.h>
#include <WICTextureLoader.h>
#include <memory>


// 頂点シェーダー読み込み
HRESULT GpuResourceUtils::LoadVertexShader(
	ID3D11Device* device,
	const std::string& filename,
	const D3D11_INPUT_ELEMENT_DESC inputElementDescs[],
	UINT inputElementCount,
	Microsoft::WRL::ComPtr<ID3D11InputLayout>& inputLayout,
	Microsoft::WRL::ComPtr<ID3D11VertexShader>& vertexShader)
{
	// ファイルを開く
	FILE* fp = nullptr;
	fopen_s(&fp, filename.c_str(), "rb");
	_ASSERT_EXPR_A(fp, "VertexShader File not found");

	// ファイルサイズを求める
	fseek(fp, 0, SEEK_END);
	long size{ ftell(fp) };
	fseek(fp, 0, SEEK_SET);

	//メモリ上に頂点データを格納する領域を用意する
	std::unique_ptr<u_char[]> cso_data = std::make_unique<u_char[]>(size);
	fread(cso_data.get(), size, 1, fp);
	fclose(fp);

	// 頂点シェーダー生成
	HRESULT hr = device->CreateVertexShader(cso_data.get(), size, nullptr, vertexShader.GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	//入力レイアウト
	//if (inputLayout != nullptr)
	if (inputElementDescs != nullptr && inputElementCount > 0)
	{
		hr = device->CreateInputLayout(inputElementDescs, inputElementCount, cso_data.get(), size, inputLayout.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
	}
	return hr;
}

// ピクセルシェーダー読み込み
HRESULT GpuResourceUtils::LoadPixelShader(
	ID3D11Device* device,
	const std::string& filename,
	Microsoft::WRL::ComPtr<ID3D11PixelShader>& pixelShader)
{
	// ファイルを開く
	FILE* fp = nullptr;
	fopen_s(&fp, filename.c_str(), "rb");
	_ASSERT_EXPR_A(fp, "PixelShader File not found");

	// ファイルサイズを求める
	fseek(fp, 0, SEEK_END);
	long size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	//メモリ上にピクセルデータを格納する領域を用意する
	std::unique_ptr<u_char[]> data = std::make_unique<u_char[]>(size);
	fread(data.get(), size, 1, fp);
	fclose(fp);

	//ピクセルシェーダー生成
	HRESULT hr = device->CreatePixelShader(data.get(), size, nullptr, pixelShader.GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	return hr;
}

// ファイルパスからテクスチャ読み込み
HRESULT GpuResourceUtils::LoadTexture(
	ID3D11Device* device,
	const std::string& filename,
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& shaderResourceView,
	D3D11_TEXTURE2D_DESC* texture2dDesc
)
{
	// std::stringをstd::wstring変換
	std::wstring wfilename = ToWString(filename);

	HRESULT hr{ S_OK };
	Microsoft::WRL::ComPtr<ID3D11Resource> resource;

	// テクスチャファイル読み込み
	hr = DirectX::CreateWICTextureFromFile(device, wfilename.c_str(), resource.GetAddressOf(), shaderResourceView.GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	//テクスチャ情報取得
	if (texture2dDesc != nullptr)
	{
		Microsoft::WRL::ComPtr<ID3D11Texture2D> texture2d;
		hr = resource->QueryInterface<ID3D11Texture2D>(texture2d.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
		texture2d->GetDesc(texture2dDesc);
	}
	return hr;
}

// 埋め込みテクスチャ読み込み
HRESULT GpuResourceUtils::LoadTexture(
	ID3D11Device* device,
	const unsigned char* data,
	size_t dataSize,
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& shaderResourceView
)
{
	HRESULT hr = DirectX::CreateWICTextureFromMemory(
		device, data, dataSize, nullptr,
		shaderResourceView.GetAddressOf());


	// 失敗時はダミーテクスチャで代替
	if (FAILED(hr))
	{
		hr = CreateDummyTexture(device, shaderResourceView, 0xFFFFFFFF, 1);
	}
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	return hr;
}

// DDSテクスチャ読み込み
HRESULT GpuResourceUtils::LoadDDSTexture(
	ID3D11Device* device,
	const std::string& filename,
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& shaderResourceView,
	bool* isCubemap
)
{
	// std::stringをstd::wstring変換
	std::wstring wfilename = ToWString(filename);

	Microsoft::WRL::ComPtr<ID3D11Resource> resource;
	HRESULT hr = DirectX::CreateDDSTextureFromFile(device, wfilename.c_str(), resource.GetAddressOf(), shaderResourceView.GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	// キューブマップ判定
	if (isCubemap != nullptr)
	{
		*isCubemap = false;
		Microsoft::WRL::ComPtr<ID3D11Texture2D> tex;
		if (SUCCEEDED(resource.As(&tex)))
		{
			D3D11_TEXTURE2D_DESC desc{};
			tex->GetDesc(&desc);
			*isCubemap = (desc.MiscFlags & D3D11_RESOURCE_MISC_TEXTURECUBE) != 0;
		}
	}
	return hr;
}

// ダミーテクスチャ生成
HRESULT GpuResourceUtils::CreateDummyTexture(
	ID3D11Device* device,
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& shaderResourceView,
	DWORD value,
	UINT dimension
)
{
	HRESULT hr = S_OK;

	D3D11_TEXTURE2D_DESC desc{};
	desc.Width = dimension;
	desc.Height = dimension;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	size_t texels = static_cast<size_t>(dimension) * dimension;
	std::unique_ptr<DWORD[]> sysmem = std::make_unique<DWORD[]>(texels);
	for (size_t i = 0; i < texels; ++i)
	{
		sysmem[i] = value;
	}

	D3D11_SUBRESOURCE_DATA subresource{};
	subresource.pSysMem = sysmem.get();
	subresource.SysMemPitch = sizeof(DWORD) * dimension;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> texture2d;
	hr = device->CreateTexture2D(&desc, &subresource, texture2d.GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = desc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	hr = device->CreateShaderResourceView(texture2d.Get(), &srvDesc, shaderResourceView.GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	return hr;
}

// 定数バッファ生成
HRESULT GpuResourceUtils::CreateConstantBuffer(
	ID3D11Device* device,
	UINT byteWidth,
	Microsoft::WRL::ComPtr<ID3D11Buffer>& constantBuffer
)
{
	// 16バイトアライメント確認
	_ASSERT_EXPR_A(byteWidth % 16 == 0, "Constant buffer size must be a multiple of 16 bytes.");

	D3D11_BUFFER_DESC desc{};
	desc.ByteWidth = byteWidth;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	desc.CPUAccessFlags = 0;

	return device->CreateBuffer(&desc, nullptr, constantBuffer.GetAddressOf());
}
