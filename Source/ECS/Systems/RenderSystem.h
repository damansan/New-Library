#pragma once
#include <d3d11.h>
#include <wrl.h>

#include "entt.hpp"

#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/StaticMeshComponent.h"
#include "ECS/Components/SkinnedMeshComponent.h"
#include "ECS/Components/MaterialComponent.h"
#include "ECS/Components/Tags.h"

#include "Resource/Model.h"


// 定数バッファ構造体
struct CbScene
{
    DirectX::XMFLOAT4X4 view;             // ビュー行列
    DirectX::XMFLOAT4X4 projection;       // プロジェクション行列
    DirectX::XMFLOAT4   lightDirection;   // ライト方向（w未使用）
    DirectX::XMFLOAT4   lightColor;       // ライト色（w=強度）
    DirectX::XMFLOAT4   ambientColor;     // 環境光色（w=強度）
    DirectX::XMFLOAT4   cameraPosition;   // カメラ位置（w未使用）
};

struct CbObject
{
    DirectX::XMFLOAT4X4 world;      // ワールド行列
};

#define MAX_BONES 256
struct CbSkinning
{
    DirectX::XMFLOAT4X4 boneTransforms[MAX_BONES];
};


class RenderSystem
{
public:
    // 初期化(シェーダー読み込み）
    void Initialize(ID3D11Device* device);

    // 描画
    void Render(
        entt::registry& registry,
        ID3D11DeviceContext* dc,
        ID3D11RenderTargetView* rtv,
        ID3D11DepthStencilView* dsv
    );

    // 定数バッファゲッター
    ID3D11Buffer* GetCbScene() { return cbScene.Get(); }
    ID3D11Buffer** GetCbSceneAddress() { return cbScene.GetAddressOf(); }
private:
    // 静的メッシュ描画
    void RenderStatic(
        entt::registry& registry,
        ID3D11DeviceContext* dc,
        const std::string& passName
    );

    // スキニングメッシュ描画
    void RenderSkinned(
        entt::registry& registry,
        ID3D11DeviceContext* dc,
        const std::string& passName
    );

    // 頂点シェーダー
    Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader;
    // スキニング用頂点シェーダー
    Microsoft::WRL::ComPtr<ID3D11VertexShader> skinnedVertexShader;

    // 入力レイアウト
    Microsoft::WRL::ComPtr<ID3D11InputLayout>  inputLayout;

    // ピクセルシェーダー
    std::unordered_map<std::string,
        Microsoft::WRL::ComPtr<ID3D11PixelShader>> pixelShaders;

    // 定数バッファ
    Microsoft::WRL::ComPtr<ID3D11Buffer> cbScene;    // シーン情報
    Microsoft::WRL::ComPtr<ID3D11Buffer> cbObject;   // オブジェクト情報
    Microsoft::WRL::ComPtr<ID3D11Buffer> cbSkinning; // スキニング定数バッファ

    // サンプラーステート
    Microsoft::WRL::ComPtr<ID3D11SamplerState>      samplerState;
    // ラスタライザーステート
    Microsoft::WRL::ComPtr<ID3D11RasterizerState>   rasterizerState;
    // 深度ステンシルステート
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depthStencilState;
    // ブレンドステート
    Microsoft::WRL::ComPtr<ID3D11BlendState>        blendState;
};