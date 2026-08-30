#pragma once
#include <d3d11.h>
#include <wrl.h>

#include "Camera.h"

class GameWindow
{
public:
    GameWindow() = default;
    ~GameWindow() = default;

    GameWindow(const GameWindow&) = delete;                // コピーコンストラクタ禁止
    GameWindow& operator=(const GameWindow&) = delete;     // コピー代入禁止
    GameWindow(GameWindow&&) noexcept = delete;            // ムーブコンストラクタ禁止
    GameWindow& operator=(GameWindow&&) noexcept = delete; // ムーブ代入禁止

    // 初期化
    void Initialize(ID3D11Device* device, UINT width = 1920, UINT height = 1080);

    // 描画先の切り替え
    void Begin(ID3D11DeviceContext* dc);
    void End(ID3D11DeviceContext* dc);

    // ImGui描画
    void DrawWindow(const char* windowName, bool& vsync);

    // テクスチャの再生成
    void ResizeRenderTarget(UINT width, UINT height);

    // カメラ取得
    Camera& GetCamera() { return camera; }

    // SRV取得
    ID3D11ShaderResourceView* GetSRV() { return srv.Get(); }

    // SceneContextへ渡すための描画先取得
    ID3D11RenderTargetView* GetRTV() { return rtv.Get(); }
    ID3D11DepthStencilView* GetDSV() { return dsv.Get(); }
    UINT GetRenderWidth()  const { return renderWidth; }
    UINT GetRenderHeight() const { return renderHeight; }

    // ビューポートがホバーされているか
    bool IsViewportHovered() const { return viewportHovered; }

private:
    Microsoft::WRL::ComPtr<ID3D11Device>             device;
    Microsoft::WRL::ComPtr<ID3D11Texture2D>          renderTexture;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView>   rtv;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
    Microsoft::WRL::ComPtr<ID3D11Texture2D>          depthTexture;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView>   dsv;

    Camera camera;

    UINT renderWidth = 1920;
    UINT renderHeight = 1080;

    bool viewportHovered = false;

    // Begin/End間でのビューポート退避用
    D3D11_VIEWPORT savedViewport = {};

    // trueの間、次のDrawWindow()でDockSpaceのレイアウトを（再）構築する
    bool firstDockSetup = true;

    // ドッキング比率（実行中にメニューのスライダーで調整可能）
    float dockRatioBottom = 0.2f;  // Project
    float dockRatioLeft = 0.1f;  // Hierarchy
    float dockRatioRight = 0.15f; // Inspector
};