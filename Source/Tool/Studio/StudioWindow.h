#pragma once
#include <d3d11.h>
#include <wrl.h>
#include <vector>
#include <memory>

#include "StudioCamera.h"
#include "StudioContext.h"
#include "IStudioAttachment.h"

#include "entt.hpp"
#include "ECS/Systems/RenderSystem.h"
#include "Rendering/LightManager.h"
#include "Resource/ResourceManager.h"

#include "StudioSharedContext.h"
#include "StudioModelViewer.h"

class StudioWindow
{
public:
    void Initialize(ID3D11Device* device, StudioSharedContext* shared, UINT width = 1280, UINT height = 720);

    // 更新
    void Update(float elapsedTime);

    // 描画
    void Render();

    // ImGui描画
    void DrawWindow(const char* windowName = "Studio");

    // アタッチメント追加
    void AddAttachment(std::unique_ptr<IStudioAttachment> attachment);

    // contextへのアクセス
    StudioContext& GetContext() { return context; }

    // アセット読み込み
    void LoadAsset(const std::string& assetPath);

    // 削除リクエスト
    bool IsCloseRequested() const { return closeRequested; }

private:
    // 描画先の切り替え
    void Begin();
    void End();
    void ResizeRenderTarget();

    Microsoft::WRL::ComPtr<ID3D11Texture2D>          renderTexture;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView>   rtv;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
    Microsoft::WRL::ComPtr<ID3D11Texture2D>          depthTexture;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView>   dsv;

    // カメラ
    StudioCamera camera;

    // アタッチメント群
    std::vector<std::unique_ptr<IStudioAttachment>> attachments;

    // アタッチメント間共有データ
    StudioContext context;

    entt::registry    studioRegistry;
    RenderSystem      renderSystem;
    LightManager      lightManager;
    ResourceManager   resourceManager;
    
    StudioSharedContext* shared = nullptr;
    StudioModelViewer modelViewer;

    // ウィンドウ状態
    UINT  renderWidth = 1280;
    UINT  renderHeight = 720;
    bool  isHovered = false;
    bool  closeRequested = false;

    // リサイズ用デバウンス
    float resizeTimer = 0.0f;
    bool  needsResize = false;
    UINT  pendingWidth = 0;
    UINT  pendingHeight = 0;

    // ビューポート保存
    D3D11_VIEWPORT savedViewport = {};
    int            renderSkipCount = 0;

};