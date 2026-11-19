#include "PostProcess.h"
#include "Engine/Resource/GpuResourceUtils.h"

#include <cmath>

namespace
{
	// b5：GPU に送る形（並びは ShaderConstants.hlsli の CbPostProcess と同じにする）
	struct CbPostProcess
	{
		float exposure;
		int   tonemapEnabled; // bool は HLSL と大きさが違うので int で送る
		float contrast;
		float saturation;

		DirectX::XMFLOAT3 colorFilter; // リニア
		float vignetteIntensity;

		DirectX::XMFLOAT3 vignetteColor; // リニア
		float vignetteSmoothness;

		float aspectRatio;
		float padding0[3];
	};

	// sRGB から リニア
	float SRGBToLinear(float c)
	{
		return (c <= 0.04045f) ? c / 12.92f : powf((c + 0.055f) / 1.055f, 2.4f);
	}

	static_assert(sizeof(CbPostProcess) % 16 == 0, "定数バッファは16バイトの倍数にする");
}

// 初期化
void PostProcess::Initialize(ID3D11Device* device)
{
	// 全画面の 頂点シェーダー
	Microsoft::WRL::ComPtr<ID3D11InputLayout> dummyLayout;
	GpuResourceUtils::LoadVertexShader(
		device, "Data/Shader/Fullscreen_VS.cso",
		nullptr, 0, dummyLayout, fullscreenVS);

	// 最終パスの ピクセルシェーダー
	GpuResourceUtils::LoadPixelShader(device, "Data/Shader/Final_PS.cso", finalPS);

	// ラスタライザー
	D3D11_RASTERIZER_DESC rd{};
	rd.FillMode = D3D11_FILL_SOLID;
	rd.CullMode = D3D11_CULL_NONE;
	device->CreateRasterizerState(&rd, rasterizerState.GetAddressOf());

	// 設定値の定数バッファ
	GpuResourceUtils::CreateConstantBuffer(device, sizeof(CbPostProcess), cbPostProcess);
}

// HDRScene を読んで output へ書き出す
void PostProcess::Render(
	ID3D11DeviceContext* dc,
	ID3D11ShaderResourceView* hdrScene,
	ID3D11RenderTargetView* output,
	UINT width,
	UINT height,
	const PostProcessSettings& settings) 
{
	// 書き出し先
	dc->OMSetRenderTargets(1, &output, nullptr);

	D3D11_VIEWPORT vp{};
	vp.Width = static_cast<float>(width);
	vp.Height = static_cast<float>(height);
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	dc->RSSetViewports(1, &vp);


	// ステート
	dc->RSSetState(rasterizerState.Get());
	dc->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);

	// 全画面の四角形
	dc->IASetInputLayout(nullptr);
	dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	dc->VSSetShader(fullscreenVS.Get(), nullptr, 0);
	dc->PSSetShader(finalPS.Get(), nullptr, 0);

	// 設定値を GPU に送る形にして渡す
	CbPostProcess cb{};
	cb.exposure = settings.exposure;
	cb.tonemapEnabled = settings.tonemapEnabled ? 1 : 0;
	cb.contrast = settings.contrast;
	cb.saturation = settings.saturation;

	// リニアに直してから送る
	cb.colorFilter =
	{
		SRGBToLinear(settings.colorFilter.x),
		SRGBToLinear(settings.colorFilter.y),
		SRGBToLinear(settings.colorFilter.z)
	};

	// ビネット
	cb.vignetteIntensity = settings.vignetteIntensity * 3.0f;
	cb.vignetteSmoothness = settings.vignetteSmoothness * 5.0f;
	cb.vignetteColor =
	{
		SRGBToLinear(settings.vignetteColor.x),
		SRGBToLinear(settings.vignetteColor.y),
		SRGBToLinear(settings.vignetteColor.z)
	};
	// 画面の縦横比
	cb.aspectRatio = (height > 0) ? static_cast<float>(width) / static_cast<float>(height) : 1.0f;

	dc->UpdateSubresource(cbPostProcess.Get(), 0, nullptr, &cb, 0, 0);
	dc->PSSetConstantBuffers(5, 1, cbPostProcess.GetAddressOf());

	// 読む物をセットして描画
	dc->PSSetShaderResources(0, 1, &hdrScene);
	dc->Draw(4, 0);

	// SRV の束縛を解除
	ID3D11ShaderResourceView* nullSrv = nullptr;
	dc->PSSetShaderResources(0, 1, &nullSrv);
}
