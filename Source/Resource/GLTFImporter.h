#pragma once

#include <memory>
#include <string>
#include <d3d11.h>
#include <filesystem>

#define TINYGLTF_NO_EXTERNAL_IMAGE
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_INCLUDE_STB_IMAGE
#define TINYGLTF_NO_INCLUDE_STB_IMAGE_WRITE
#include "tiny_gltf.h"

#include "Model.h"

class GLTFImporter
{
public:
    // モデル読み込み＆スキニング判定
    static std::shared_ptr<Model> Load(
        ID3D11Device* device,
        const std::string& filename
    );

private:
    // ノード読み込み
    static void LoadNodes(
        Model& model,
        const tinygltf::Model& gltfModel
    );

    // メッシュ読み込み
    static void LoadMeshes(
        Model& model,
        const tinygltf::Model& gltfModel,
        ID3D11Device* device
    );

    // マテリアル読み込み
    static void LoadMaterials(
        Model& model,
        const tinygltf::Model& gltfModel,
        ID3D11Device* device,
        const std::filesystem::path& filepath
    );

    // アニメーション読み込み
    static void LoadAnimations(
        Model& model,
        const tinygltf::Model& gltfModel,
        float sampleRate = 60.0f
    );

    // スキンデータ読み込み
    static void LoadSkins(
        Model& model,
        const tinygltf::Model& gltfModel
    );

    // スキニング判定
    static bool CheckHasSkin(const tinygltf::Model& gltfModel);

    // 座標系変換ヘルパー
    static DirectX::XMFLOAT3   ToXMFLOAT3(const std::vector<double>& v);
    static DirectX::XMFLOAT4   ToXMFLOAT4(const std::vector<double>& v);
    static DirectX::XMFLOAT4X4 ToXMFLOAT4X4(const std::vector<double>& v);

    // タンジェント計算
    static void ComputeTangents(
        std::vector<Model::Vertex>& vertices,
        const std::vector<uint32_t>& indices
    );

    // グローバルトランスフォーム計算
    static void CalcGlobalTransform(
        std::vector<Model::Node>& nodes,
        int nodeIndex,
        const DirectX::XMFLOAT4X4& parentTransform
    );
};