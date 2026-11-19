#include "PostProcess.h"
#include "Engine/Resource/GpuResourceUtils.h"

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
}

// HDRScene を読んで output へ書き出す
void PostProcess::Render(
	ID3D11DeviceContext* dc,
	ID3D11ShaderResourceView* hdrScene,
	ID3D11RenderTargetView* output,
	UINT width,
	UINT height)
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

	// 読む物をセットして描画
	dc->PSSetShaderResources(0, 1, &hdrScene);
	dc->Draw(4, 0);

	// SRV の束縛を解除
	ID3D11ShaderResourceView* nullSrv = nullptr;
	dc->PSSetShaderResources(0, 1, &nullSrv);
}
