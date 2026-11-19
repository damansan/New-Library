#pragma once
#include <d3d11.h>
#include <wrl.h>

// ポストプロセス
class PostProcess
{
public:
	// 初期化
	void Initialize(ID3D11Device* device);

	// HDRScene を読んで output へ書き出す
	void Render(
		ID3D11DeviceContext* dc,
		ID3D11ShaderResourceView* hdrScene,
		ID3D11RenderTargetView* output,
		UINT width,
		UINT height);

private:
	Microsoft::WRL::ComPtr<ID3D11VertexShader>    fullscreenVS;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>     finalPS;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizerState; // カリングなし
};