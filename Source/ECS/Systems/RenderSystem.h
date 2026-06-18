#pragma once
#include <d3d11.h>
#include <wrl.h>
#include <functional>

#include "entt.hpp"

#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/StaticMeshComponent.h"
#include "ECS/Components/SkinnedMeshComponent.h"
#include "ECS/Components/MaterialComponent.h"
#include "ECS/Components/Tags.h"

#include "Resource/Model.h"
#include "Rendering/GBuffer.h"

// �萔�o�b�t�@�\����
struct CbScene
{
    DirectX::XMFLOAT4X4 view;             // �r���[�s��
    DirectX::XMFLOAT4X4 projection;       // �v���W�F�N�V�����s��
    DirectX::XMFLOAT4   cameraPosition;   // �J�����ʒu�iw���g�p�j
};

struct CbObject
{
    DirectX::XMFLOAT4X4 world;      // ���[���h�s��
};

#define MAX_BONES 256
struct CbSkinning
{
    DirectX::XMFLOAT4X4 boneTransforms[MAX_BONES];
};


class RenderSystem
{
public:
    // ������(�V�F�[�_�[�ǂݍ��݁j
    void Initialize(ID3D11Device* device);

    // �`��
    void Render(
        entt::registry& registry,
        ID3D11DeviceContext* dc,
        ID3D11RenderTargetView* rtv,
        ID3D11DepthStencilView* dsv,
        ID3D11Device* device,
        UINT width,
        UINT height
    );

    // �t���X�^����Scene������Z�b�g
    void SetFrustum(
        const DirectX::XMFLOAT4X4& view,
        const DirectX::XMFLOAT4X4& projection)
    {
        cachedFrustum = CalcFrustum(view, projection);
    }

    // IBL�e�N�X�`���Z�b�g
    void SetIBLTextures(
        ID3D11ShaderResourceView* diffuseIem,
        ID3D11ShaderResourceView* specularPmrem,
        ID3D11ShaderResourceView* lutGgx)
    {
        iblDiffuseIem = diffuseIem;
        iblSpecularPmrem = specularPmrem;
        iblLutGgx = lutGgx;
    }

    // LightingPass後・ForwardTransparentPass前に呼ばれるコールバック（スカイボックス描画用）
    void SetPreTransparentCallback(std::function<void()> callback)
    {
        preTransparentCallback = std::move(callback);
    }

    // �萔�o�b�t�@�Q�b�^�[
    ID3D11Buffer* GetCbScene() { return cbScene.Get(); }
    ID3D11Buffer** GetCbSceneAddress() { return cbScene.GetAddressOf(); }

    // ��ʃT�C�Y�Z�b�g
    void SetScreenSize(UINT width, UINT height)
    {
        screenWidth = width;
        screenHeight = height;
    }
private:

    // �t���X�^������
    struct FrustumPlane
    {
        DirectX::XMFLOAT3 normal;
        float distance;
    };

    // �t���X�^���i6���ʁj
    struct Frustum
    {
        FrustumPlane planes[6]; // Left, Right, Top, Bottom, Near, Far
    };
    
    // �t���X�^���v�Z�iViewProjection�s�񂩂�j
    Frustum CalcFrustum(
        const DirectX::XMFLOAT4X4& view,
        const DirectX::XMFLOAT4X4& projection
    );

    // �o�E���f�B���O�{�b�N�X���t���X�^�����ɂ��邩����
    bool IsInFrustum(
        const Frustum& frustum,
        const Model::BoundingBox& bbox,
        const DirectX::XMFLOAT4X4& world
    );

    
    // ���_�V�F�[�_�[
    Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader;
    // �X�L�j���O�p���_�V�F�[�_�[
    Microsoft::WRL::ComPtr<ID3D11VertexShader> skinnedVertexShader;

    // ���̓��C�A�E�g
    Microsoft::WRL::ComPtr<ID3D11InputLayout>  inputLayout;

    // �萔�o�b�t�@
    Microsoft::WRL::ComPtr<ID3D11Buffer> cbScene;    // �V�[�����
    Microsoft::WRL::ComPtr<ID3D11Buffer> cbObject;   // �I�u�W�F�N�g���
    Microsoft::WRL::ComPtr<ID3D11Buffer> cbSkinning; // �X�L�j���O�萔�o�b�t�@

    // �T���v���[�X�e�[�g
    Microsoft::WRL::ComPtr<ID3D11SamplerState>      samplerState;
    // ���X�^���C�U�[�X�e�[�g
    Microsoft::WRL::ComPtr<ID3D11RasterizerState>   rasterizerState;
    // �[�x�X�e���V���X�e�[�g
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depthStencilState;
    // �������p�[�x�X�e���V���X�e�[�g
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depthStencilStateTransparent;
    // �u�����h�X�e�[�g
    Microsoft::WRL::ComPtr<ID3D11BlendState>        blendState;
    // GBuffer�p�i�u�����h�Ȃ��j
    Microsoft::WRL::ComPtr<ID3D11BlendState>        blendStateOpaque;  

    // �_�~�[�e�N�X�`��
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> dummyWhite;  // Albedo/Occlusion�p
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> dummyNormal; // �@���}�b�v�p (0.5,0.5,1.0)
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> dummyBlack;  // Emissive�p

    // IBL�p�e�N�X�`��
    ID3D11ShaderResourceView* iblDiffuseIem = nullptr;
    ID3D11ShaderResourceView* iblSpecularPmrem = nullptr;
    ID3D11ShaderResourceView* iblLutGgx = nullptr;

    // LightingPass後・ForwardTransparentPass前コールバック
    std::function<void()> preTransparentCallback;

    Frustum cachedFrustum;


    // GBuffer
    GBuffer gBuffer;

    // ��ʃT�C�Y
    UINT screenWidth = 0;
    UINT screenHeight = 0;

    // GBuffer�V�F�[�_�[�}�b�v
    std::unordered_map<std::string,
        Microsoft::WRL::ComPtr<ID3D11PixelShader>> gBufferPixelShaders;
    
    // Lighting�p�X�p
    Microsoft::WRL::ComPtr<ID3D11VertexShader> lightingVS;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>  lightingPS;

    // �t���X�N���[���`��p���X�^���C�U�[
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizerStateNoCull;

    // ForwardTransparent�p�X�p
    Microsoft::WRL::ComPtr<ID3D11PixelShader> forwardTransparentPS;

    // Geometry�p�X
    void GeometryPass(
        entt::registry& registry,
        ID3D11DeviceContext* dc,
        ID3D11DepthStencilView* dsv,
        const Frustum& frustum
    );

    // Lighting�p�X
    void LightingPass(
        ID3D11DeviceContext* dc,
        ID3D11RenderTargetView* rtv,
        ID3D11DepthStencilView* dsv
    );

    // ForwardTransparent�p�X
    void ForwardTransparentPass(
        entt::registry& registry,
        ID3D11DeviceContext* dc,
        ID3D11RenderTargetView* rtv,
        ID3D11DepthStencilView* dsv,
        const Frustum& frustum
    );
};