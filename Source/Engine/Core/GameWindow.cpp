#include "GameWindow.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <cassert>

// 初期化
void GameWindow::Initialize(ID3D11Device* dev, UINT width, UINT height)
{
    device = dev;

    // カメラ初期化
    camera.Initialize(static_cast<float>(width), static_cast<float>(height));

    ResizeRenderTarget(width, height);
}

// 描画先の切り替え
void GameWindow::Begin(ID3D11DeviceContext* dc)
{
    if (!dc) return;

    // 現在のビューポートを保存
    UINT numVP = 1;
    dc->RSGetViewports(&numVP, &savedViewport);

    // 自前のビューポートをセット
    D3D11_VIEWPORT vp{};
    vp.Width = static_cast<float>(renderWidth);
    vp.Height = static_cast<float>(renderHeight);
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    dc->RSSetViewports(1, &vp);

    // 描画先を自前RTVに切り替え
    dc->OMSetRenderTargets(1, rtv.GetAddressOf(), dsv.Get());

    // クリア
    float clear[4] = { 0.2f, 0.2f, 0.5f, 1.0f };
    dc->ClearRenderTargetView(rtv.Get(), clear);
    dc->ClearDepthStencilView(dsv.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}
void GameWindow::End(ID3D11DeviceContext* dc)
{
    if (!dc) return;

    // ビューポートを元に戻す
    dc->RSSetViewports(1, &savedViewport);

    // 描画先をメインに戻す
    ID3D11RenderTargetView* nullRTV = nullptr;
    dc->OMSetRenderTargets(1, &nullRTV, nullptr);
}

// ImGui描画
void GameWindow::DrawWindow(const char* windowName, bool& vsync)
{
    ImGui::Begin(windowName, nullptr, ImGuiWindowFlags_MenuBar);

    // メニューバー
    if (ImGui::BeginMenuBar())
    {
        ImGui::Checkbox("VSync", &vsync);

        ImGui::EndMenuBar();
    }

    // Game専用のDockSpace
    ImGuiID dockSpaceId = ImGui::GetID("GameDockSpace");
    ImGui::DockSpace(dockSpaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

    // 初回フレームだけデフォルトレイアウトを組む
    if (firstDockSetup)
    {
        firstDockSetup = false;

        ImGui::DockBuilderRemoveNode(dockSpaceId);
        ImGui::DockBuilderAddNode(dockSpaceId, ImGuiDockNodeFlags_None);
        ImGui::DockBuilderSetNodeSize(dockSpaceId, ImVec2(1280, 720));

        ImGuiID dockMain = dockSpaceId;
        ImGuiID dockBottom = ImGui::DockBuilderSplitNode(
            dockMain, ImGuiDir_Down, dockRatioBottom, nullptr, &dockMain);
        ImGuiID dockLeft = ImGui::DockBuilderSplitNode(
            dockMain, ImGuiDir_Left, dockRatioLeft, nullptr, &dockMain);
        ImGuiID dockRight = ImGui::DockBuilderSplitNode(
            dockMain, ImGuiDir_Right, dockRatioRight, nullptr, &dockMain);

        ImGui::DockBuilderDockWindow("Viewport", dockMain);
        ImGui::DockBuilderDockWindow("Hierarchy", dockLeft);
        ImGui::DockBuilderDockWindow("Inspector", dockRight);
        ImGui::DockBuilderDockWindow("Project", dockBottom);

        ImGui::DockBuilderFinish(dockSpaceId);
    }

    // Viewportパネル
    ImGui::Begin("Viewport");

    ImVec2 region = ImGui::GetContentRegionAvail();
    if (region.x > 1 && region.y > 1)
    {
        camera.SetPerspectiveFov(
            DirectX::XMConvertToRadians(camera.GetViewingAngle()),
            region.x / region.y,
            0.1f,
            1000.0f
        );
    }

    ImGui::Image(reinterpret_cast<ImTextureID>(srv.Get()), region);

    // カメラ操作の開始判定はViewport単体のホバーだけを見る
    viewportHovered = ImGui::IsWindowHovered();

    ImGui::End(); // Viewport

    ImGui::End(); // Game
}

// テクスチャの再生成
void GameWindow::ResizeRenderTarget(UINT width, UINT height)
{
    if (!device) return;
    if (width == 0 || height == 0) return;

    renderWidth = width;
    renderHeight = height;

    // 古いリソースを解放
    srv.Reset();
    rtv.Reset();
    renderTexture.Reset();
    dsv.Reset();
    depthTexture.Reset();

    HRESULT hr;

    // カラーテクスチャ再生成
    {
        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = renderWidth;
        desc.Height = renderHeight;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

        hr = device->CreateTexture2D(&desc, nullptr, renderTexture.GetAddressOf());
        assert(SUCCEEDED(hr));
        hr = device->CreateRenderTargetView(renderTexture.Get(), nullptr, rtv.GetAddressOf());
        assert(SUCCEEDED(hr));
        hr = device->CreateShaderResourceView(renderTexture.Get(), nullptr, srv.GetAddressOf());
        assert(SUCCEEDED(hr));
    }

    // 深度テクスチャ再生成
    {
        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = renderWidth;
        desc.Height = renderHeight;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

        hr = device->CreateTexture2D(&desc, nullptr, depthTexture.GetAddressOf());
        assert(SUCCEEDED(hr));
        hr = device->CreateDepthStencilView(depthTexture.Get(), nullptr, dsv.GetAddressOf());
        assert(SUCCEEDED(hr));
    }
}