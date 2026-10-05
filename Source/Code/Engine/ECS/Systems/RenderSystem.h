#pragma once
#include <d3d11.h>
#include <wrl.h>

#include "entt.hpp"

#include "Engine/ECS/Components/TransformComponent.h"
#include "Engine/ECS/Components/StaticMeshComponent.h"
#include "Engine/ECS/Components/SkinnedMeshComponent.h"
#include "Engine/ECS/Components/MaterialComponent.h"
#include "Engine/ECS/Components/Tags.h"

#include "Engine/Resource/Model.h"
#include "Engine/Rendering/GBuffer.h"

// 定数バッファ構造体
struct CbScene
{
	DirectX::XMFLOAT4X4 view;             // ビュー行列
	DirectX::XMFLOAT4X4 projection;       // プロジェクション行列
	DirectX::XMFLOAT4   cameraPosition;   // カメラ位置（w未使用）
};

struct CbObject
{
	DirectX::XMFLOAT4X4 world;       // ワールド行列
	float               alphaCutoff; // Mask素材のクリップしきい値
	DirectX::XMFLOAT3   padding;     // 16バイトアライメント用
};

#define MAX_BONES 256

struct CbSkinning
{
	DirectX::XMFLOAT4X4 boneTransforms[MAX_BONES];
};


class RenderSystem
{
public:
	// 初期化
	void Initialize(ID3D11Device* device);

	// 描画
	void Render(
		entt::registry& registry,
		ID3D11DeviceContext* dc,
		ID3D11RenderTargetView* rtv,
		ID3D11DepthStencilView* dsv,
		ID3D11Device* device,
		UINT width,
		UINT height
	);

	// フラスタムをScene側からセット
	void SetFrustum(
		const DirectX::XMFLOAT4X4& view,
		const DirectX::XMFLOAT4X4& projection)
	{
		cachedFrustum = CalcFrustum(view, projection);
	}

	// IBLテクスチャセット
	void SetIBLTextures(
		ID3D11ShaderResourceView* diffuseIem,
		ID3D11ShaderResourceView* specularPmrem,
		ID3D11ShaderResourceView* lutGgx)
	{
		iblDiffuseIem = diffuseIem;
		iblSpecularPmrem = specularPmrem;
		iblLutGgx = lutGgx;
	}

	// 定数バッファゲッター
	ID3D11Buffer* GetCbScene() { return cbScene.Get(); }
	ID3D11Buffer** GetCbSceneAddress() { return cbScene.GetAddressOf(); }

private:

	// フラスタム平面
	struct FrustumPlane
	{
		DirectX::XMFLOAT3 normal;
		float distance;
	};

	// フラスタム（6平面）
	struct Frustum
	{
		FrustumPlane planes[6]; // Left, Right, Top, Bottom, Near, Far
	};

	// フラスタム計算（ViewProjection行列から）
	Frustum CalcFrustum(
		const DirectX::XMFLOAT4X4& view,
		const DirectX::XMFLOAT4X4& projection
	);

	// バウンディングボックスがフラスタム内にあるか判定
	bool IsInFrustum(
		const Frustum& frustum,
		const Model::BoundingBox& bbox,
		const DirectX::XMFLOAT4X4& world
	);


	// Geometryパス
	void GeometryPass(
		entt::registry& registry,
		ID3D11DeviceContext* dc,
		ID3D11DepthStencilView* dsv,
		const Frustum& frustum
	);

	// Lightingパス
	void LightingPass(
		ID3D11DeviceContext* dc,
		ID3D11RenderTargetView* rtv,
		ID3D11DepthStencilView* dsv
	);

	// 半透明パス(フォワードレンダリング)
	void ForwardTransparentPass(
		entt::registry& registry,
		ID3D11DeviceContext* dc,
		ID3D11RenderTargetView* rtv,
		ID3D11DepthStencilView* dsv,
		const Frustum& frustum
	);


	Microsoft::WRL::ComPtr<ID3D11VertexShader> staticVertexShader;   // 静的メッシュ用
	Microsoft::WRL::ComPtr<ID3D11VertexShader> skinnedVertexShader;  // スキニングメッシュ用
	Microsoft::WRL::ComPtr<ID3D11VertexShader> lightingVS;           // Lightingパス用頂点シェーダー
	Microsoft::WRL::ComPtr<ID3D11PixelShader>  lightingPS;           // Lightingパス用ピクセルシェーダー
	Microsoft::WRL::ComPtr<ID3D11PixelShader>  forwardTransparentPS; // 半透明フォワード用(PBR)

	// GBufferシェーダーマップ
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D11PixelShader>> gBufferPixelShaders;
	
	// 入力レイアウト
	Microsoft::WRL::ComPtr<ID3D11InputLayout>  inputLayout;



	// サンプラーステート
	Microsoft::WRL::ComPtr<ID3D11SamplerState>      samplerState;
	// ラスタライザーステート
	Microsoft::WRL::ComPtr<ID3D11RasterizerState>   rasterizerState;
	// ラスタライザーステート(フルスクリーン描画用)
	Microsoft::WRL::ComPtr<ID3D11RasterizerState>   rasterizerStateNoCull;

	// 深度ステンシルステート
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depthStencilState;  
	// 深度ステンシルステート(半透明用)
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depthStencilStateTransparent; 
	
	// GeometryPass
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depthStencilStateGeometry;   
	// LightingPass
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depthStencilStateLighting;

	// アルファブレンド(半透明用)
	Microsoft::WRL::ComPtr<ID3D11BlendState> blendState;
	// GBuffer用(ブレンドなし)
	Microsoft::WRL::ComPtr<ID3D11BlendState> blendStateOpaque;


	// 定数バッファ
	Microsoft::WRL::ComPtr<ID3D11Buffer> cbScene;    // シーン情報
	Microsoft::WRL::ComPtr<ID3D11Buffer> cbObject;   // オブジェクト情報
	Microsoft::WRL::ComPtr<ID3D11Buffer> cbSkinning; // スキニング定数バッファ


	// ダミーテクスチャ
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> dummyWhite;  // Albedo/Occlusion用
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> dummyNormal; // 法線マップ用
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> dummyBlack;  // Emissive用

	// IBL用テクスチャ
	ID3D11ShaderResourceView* iblDiffuseIem    = nullptr;
	ID3D11ShaderResourceView* iblSpecularPmrem = nullptr;
	ID3D11ShaderResourceView* iblLutGgx        = nullptr;


	// フラスタム
	Frustum cachedFrustum;
	// GBuffer
	GBuffer gBuffer;

	// 画面サイズ
	UINT screenWidth = 0;
	UINT screenHeight = 0;
};