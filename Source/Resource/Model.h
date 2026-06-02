#pragma once

#include <string>
#include <vector>
#include <DirectXMath.h>
#include <wrl.h>
#include <d3d11.h>

class Model
{
public:

	// 頂点データ
    struct Vertex
    {
        DirectX::XMFLOAT3 position = { 0, 0, 0 };
        DirectX::XMFLOAT3 normal = { 0, 0, 0 };
        DirectX::XMFLOAT4 tangent = { 0, 0, 0, 1 };
        DirectX::XMFLOAT2 texcoord = { 0, 0 };
        DirectX::XMFLOAT4 boneWeight = { 1, 0, 0, 0 };
        DirectX::XMUINT4  boneIndex = { 0, 0, 0, 0 };
    };

    // ボーン
    struct Bone
    {
        int nodeIndex;
        DirectX::XMFLOAT4X4 offsetTransform;
    };

    // ノード
    struct Node
    {
        std::string         name;
        int                 parentIndex = -1;
        std::vector<int>    childIndices;
        DirectX::XMFLOAT3   position = { 0, 0, 0 };
        DirectX::XMFLOAT4   rotation = { 0, 0, 0, 1 };
        DirectX::XMFLOAT3   scale = { 1, 1, 1 };
        DirectX::XMFLOAT4X4 localTransform = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
        DirectX::XMFLOAT4X4 globalTransform = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
    };

    // マテリアル
    struct Material
    {
        std::string       name;
        std::string       baseTextureFileName;
        DirectX::XMFLOAT4 baseColor = { 1, 1, 1, 1 };
        float             roughnessFactor = 1.0f;
        float             metallicFactor = 0.0f;

        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> baseMap;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> normalMap;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> metallicRoughnessMap;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> occlusionMap;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> emissiveMap;

        enum class AlphaMode { Opaque, Blend, Mask };
        AlphaMode alphaMode = AlphaMode::Opaque;
        float     alphaCutoff = 0.5f;
    };

    // メッシュ
    struct Mesh
    {
        std::vector<Vertex>   vertices;
        std::vector<uint32_t> indices;
        std::vector<Bone>     bones;
        int      materialIndex = 0;
        int      skinIndex = -1;
        Material* material = nullptr;

        Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
        Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;
    };

    // アニメーションキーフレーム
    struct VectorKeyframe
    {
        float               seconds;
        DirectX::XMFLOAT3   value;
    };
    struct QuaternionKeyframe
    {
        float               seconds;
        DirectX::XMFLOAT4   value;
    };

    // ノードアニメーション
    struct NodeAnim
    {
        std::vector<VectorKeyframe>     positionKeyframes;
        std::vector<QuaternionKeyframe> rotationKeyframes;
        std::vector<VectorKeyframe>     scaleKeyframes;
    };

    // アニメーション
    struct Animation
    {
        std::string           name;
        float                 secondsLength;
        std::vector<NodeAnim> nodeAnims;
    };

    struct Skin
    {
        std::vector<int>                  joints;             // jointIndex → nodeIndex の対応表
        std::vector<DirectX::XMFLOAT4X4> inverseBindMatrices; // joint数分のオフセット行列
    };

    // 入力レイアウト
    static const std::vector<D3D11_INPUT_ELEMENT_DESC> InputElementDescs;

    // スキニングの有無
    bool HasSkin() const { return hasSkin; }


    // データアクセサー
    const std::vector<Node>& GetNodes()      const { return nodes; }
    const std::vector<Mesh>& GetMeshes()     const { return meshes; }
    const std::vector<Material>& GetMaterials()  const { return materials; }
    const std::vector<Animation>& GetAnimations() const { return animations; }
    const std::vector<Skin>& GetSkins()      const { return skins; }  // ← 追加

    std::vector<Node>& GetNodes() { return nodes; }
    std::vector<Mesh>& GetMeshes() { return meshes; }
    std::vector<Material>& GetMaterials() { return materials; }

private:
    friend class GLTFImporter;

    std::vector<Node>      nodes;
    std::vector<Mesh>      meshes;
    std::vector<Material>  materials;
    std::vector<Animation> animations;
    std::vector<Skin>      skins;
    bool                   hasSkin = false;
};