#pragma once
#include <d3d11.h>
#include <wrl.h>
#include <DirectXMath.h>

// シャドウマップクラス
class ShadowMap
{
public:
	// カスケード分割数
	static constexpr int  CascadeCount = 4;
	// シャドウマップ解像度
	static constexpr UINT ShadowMapSize = 2048;

	// カスケード分割距離テーブル
	static constexpr float SplitDistances[CascadeCount + 1] =
	{
		0.1f,   // 近景 ：近平面(カメラのニアクリップに合わせる)
		25.0f,  // 近景 ：遠平面 / 中景1：近平面
		100.0f, // 中景1：遠平面 / 中景2：近平面
		250.0f, // 中景2：遠平面 / 遠景：近平面
		500.0f, // 遠景 ：遠平面(これを超えると影は出ない)
	};

	// 初期化
	void Initialize(ID3D11Device* device);

	// カスケードごとのライトビュープロジェクション行列を算出
	void UpdateMatrices(
		const DirectX::XMFLOAT3& cameraPosition,
		const DirectX::XMFLOAT3& cameraFront,
		const DirectX::XMFLOAT3& cameraRight,
		const DirectX::XMFLOAT3& cameraUp,
		float fovY,   // ラジアン
		float aspect,
		const DirectX::XMFLOAT3& lightDirection
	);

	// 指定カスケードのDSVを取得
	ID3D11DepthStencilView* GetDSV(int cascadeIndex) const
	{
		return dsvs[cascadeIndex].Get();
	}

	// 全カスケードをまとめたSRV(Texture2DArray)のアドレスを取得(PSSetShaderResources用)
	ID3D11ShaderResourceView* const* GetSRVAddress() const { return &srvRaw; }

	// 指定カスケードのライトビュープロジェクション行列を取得
	const DirectX::XMFLOAT4X4& GetLightViewProjection(int cascadeIndex) const
	{
		return lightViewProjections[cascadeIndex];
	}

private:
	// 1枚のTexture2DArray(ArraySize=CascadeCount)として深度バッファを持つ
	Microsoft::WRL::ComPtr<ID3D11Texture2D>          texture;
	// カスケードごとのDSV(該当スライス1枚だけを描画対象にするビュー)
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView>   dsvs[CascadeCount];
	// 全カスケードをまとめて読むSRV(Texture2DArray、1つでOK)
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
	// PSSetShaderResourcesに渡すためのrawポインタ(1個だけの配列)
	ID3D11ShaderResourceView* srvRaw = nullptr;

	// カスケードごとのライトビュープロジェクション行列
	DirectX::XMFLOAT4X4 lightViewProjections[CascadeCount]{};
};