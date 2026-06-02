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
#include "GLTFConverter.h"

class GLTFImporter
{
public:
    // モデル読み込み
    static std::shared_ptr<Model> Load(
        ID3D11Device* device,
        const std::string& filename
    );

    // グローバルトランスフォーム計算
    static void CalcGlobalTransform(
        std::vector<Model::Node>& nodes,
        int nodeIndex,
        const DirectX::XMFLOAT4X4& parentTransform
    );

private:
    // ノード読み込み + 右手系→左手系変換
    static void LoadNodes(
        Model& model,
        const tinygltf::Model& gltfModel
    );

    // メッシュ読み込み + 右手系→左手系変換
    static void LoadMeshes(
        Model& model,
        const tinygltf::Model& gltfModel,
        ID3D11Device* device
    );

    // マテリアル読み込み（座標系変換なし）
    static void LoadMaterials(
        Model& model,
        const tinygltf::Model& gltfModel,
        ID3D11Device* device,
        const std::filesystem::path& filepath
    );

    // スキン読み込み + 右手系→左手系変換
    static void LoadSkins(
        Model& model,
        const tinygltf::Model& gltfModel
    );

    // アニメーション読み込み + 右手系→左手系変換
    static void LoadAnimations(
        Model& model,
        const tinygltf::Model& gltfModel
    );

    // スキニング判定
    static bool CheckHasSkin(const tinygltf::Model& gltfModel);

    // タンジェント計算
    static void ComputeTangents(
        std::vector<Model::Vertex>& vertices,
        const std::vector<uint32_t>& indices
    );
};