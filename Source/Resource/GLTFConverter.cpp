#include "GLTFConverter.h"
#include "GpuResourceUtils.h"
#include "Utility/misc.h"

// GLTFData を受け取り Model を生成して返す
std::shared_ptr<Model> GLTFConverter::Convert(
	ID3D11Device* device,
	const std::shared_ptr<GLTFData>& data)
{
	auto model = std::make_shared<Model>();
	model->hasSkin = data->hasSkin;

	ConvertNodes(*model, *data);
	ConvertMeshes(*model, *data, device);
	ConvertMaterials(*model, *data, device);
	ConvertAnimations(*model, *data);

	return model;
}

// ノード変換
void GLTFConverter::ConvertNodes(
	Model& model,
	const GLTFData& data)
{
	model.nodes.reserve(data.nodes.size());

	for (const auto& src : data.nodes)
	{
		Model::Node dst;
		dst.name = src.name;
		dst.parentIndex = src.parentIndex;
		dst.childIndices = src.childIndices;
		dst.position = src.position;
		dst.rotation = src.rotation;
		dst.scale = src.scale;
		dst.localTransform = src.localTransform;
		dst.globalTransform = src.globalTransform;

		model.nodes.push_back(std::move(dst));
	}
}

// メッシュ変換
void GLTFConverter::ConvertMeshes(
	Model& model,
	const GLTFData& data, 
	ID3D11Device* device)
{
    model.meshes.reserve(data.meshes.size());

    for (const auto& src : data.meshes)
    {
        Model::Mesh dst;
        dst.nodeIndex = src.nodeIndex;
        dst.materialIndex = src.materialIndex;

        // 頂点データをGLTFData::VertexからModel::Vertexへコピー
        dst.vertices.reserve(src.vertices.size());
        for (const auto& v : src.vertices)
        {
            Model::Vertex mv;
            mv.position = v.position;
            mv.normal = v.normal;
            mv.tangent = v.tangent;
            mv.texcoord = v.texcoord;
            mv.boneWeight = v.boneWeight;
            mv.boneIndex = v.boneIndex;
            dst.vertices.push_back(mv);
        }

        // インデックスデータコピー
        dst.indices = src.indices;

        // ボーンデータコピー
        dst.bones.reserve(src.bones.size());
        for (const auto& b : src.bones)
        {
            Model::Bone mb;
            mb.nodeIndex = b.nodeIndex;
            mb.offsetTransform = b.offsetTransform;
            dst.bones.push_back(mb);
        }

        // 頂点バッファ生成
        {
            D3D11_BUFFER_DESC desc{};
            desc.ByteWidth = static_cast<UINT>(
                sizeof(Model::Vertex) * dst.vertices.size());
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

            D3D11_SUBRESOURCE_DATA sub{};
            sub.pSysMem = dst.vertices.data();

            HRESULT hr = device->CreateBuffer(
                &desc, &sub, dst.vertexBuffer.GetAddressOf());
            _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
        }

        // インデックスバッファ生成
        {
            D3D11_BUFFER_DESC desc{};
            desc.ByteWidth = static_cast<UINT>(
                sizeof(uint32_t) * dst.indices.size());
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.BindFlags = D3D11_BIND_INDEX_BUFFER;

            D3D11_SUBRESOURCE_DATA sub{};
            sub.pSysMem = dst.indices.data();

            HRESULT hr = device->CreateBuffer(
                &desc, &sub, dst.indexBuffer.GetAddressOf());
            _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
        }

        model.meshes.push_back(std::move(dst));
    }
    // 全メッシュの頂点からバウンディングボックスを計算
    for (const auto& mesh : model.meshes)
    {
        for (const auto& v : mesh.vertices)
        {
            model.boundingBox.min.x = (std::min)(model.boundingBox.min.x, v.position.x);
            model.boundingBox.min.y = (std::min)(model.boundingBox.min.y, v.position.y);
            model.boundingBox.min.z = (std::min)(model.boundingBox.min.z, v.position.z);

            model.boundingBox.max.x = (std::max)(model.boundingBox.max.x, v.position.x);
            model.boundingBox.max.y = (std::max)(model.boundingBox.max.y, v.position.y);
            model.boundingBox.max.z = (std::max)(model.boundingBox.max.z, v.position.z);
        }
    }
}

// マテリアル変換
void GLTFConverter::ConvertMaterials(
	Model& model,
	const GLTFData& data, 
	ID3D11Device* device)
{
    model.materials.reserve(data.materials.size());

    // テクスチャ変換ヘルパー
    // インデックスが -1 ならダミーテクスチャを生成する
    auto convertTexture = [&](
        int texIndex,
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& srv,
        DWORD dummyColor)
        {
            if (texIndex < 0 ||
                texIndex >= (int)data.textures.size())
            {
                GpuResourceUtils::CreateDummyTexture(
                    device, srv, dummyColor, 1);
                return;
            }

            const auto& texData = data.textures[texIndex];
            if (texData.pixels.empty())
            {
                GpuResourceUtils::CreateDummyTexture(
                    device, srv, dummyColor, 1);
                return;
            }

            // バイナリデータからテクスチャ生成
            GpuResourceUtils::LoadTexture(
                device,
                texData.pixels.data(),
                texData.pixels.size(),
                srv);
        };

    for (const auto& src : data.materials)
    {
        Model::Material dst;
        dst.name = src.name;
        dst.baseColor = src.baseColor;
        dst.roughnessFactor = src.roughnessFactor;
        dst.metallicFactor = src.metallicFactor;
        dst.alphaMode = static_cast<Model::Material::AlphaMode>(
            src.alphaMode);
        dst.alphaCutoff = src.alphaCutoff;

        // テクスチャ生成
        convertTexture(src.baseMapIndex,
            dst.baseMap, 0xFFFFFFFF);
        convertTexture(src.normalMapIndex,
            dst.normalMap, 0xFFFF8080);
        convertTexture(src.metallicRoughnessMapIndex,
            dst.metallicRoughnessMap, 0xFFFFFFFF);
        convertTexture(src.occlusionMapIndex,
            dst.occlusionMap, 0xFFFFFFFF);
        convertTexture(src.emissiveMapIndex,
            dst.emissiveMap, 0xFF000000);

        model.materials.push_back(std::move(dst));
    }

    // メッシュとマテリアルを紐付け
    for (auto& mesh : model.meshes)
    {
        if (mesh.materialIndex >= 0 &&
            mesh.materialIndex < (int)model.materials.size())
        {
            mesh.material = &model.materials[mesh.materialIndex];
        }
    }
}

// アニメーション変換
void GLTFConverter::ConvertAnimations(
	Model& model, 
	const GLTFData& data)
{
    model.animations.reserve(data.animations.size());

    for (const auto& src : data.animations)
    {
        Model::Animation dst;
        dst.name = src.name;
        dst.secondsLength = src.secondsLength;

        dst.nodeAnims.reserve(src.nodeAnims.size());
        for (const auto& srcNode : src.nodeAnims)
        {
            Model::NodeAnim dstNode;

            // positionキーフレーム
            dstNode.positionKeyframes.reserve(
                srcNode.positionKeyframes.size());
            for (const auto& kf : srcNode.positionKeyframes)
            {
                Model::VectorKeyframe dkf;
                dkf.seconds = kf.seconds;
                dkf.value = kf.value;
                dstNode.positionKeyframes.push_back(dkf);
            }

            // rotationキーフレーム
            dstNode.rotationKeyframes.reserve(
                srcNode.rotationKeyframes.size());
            for (const auto& kf : srcNode.rotationKeyframes)
            {
                Model::QuaternionKeyframe dkf;
                dkf.seconds = kf.seconds;
                dkf.value = kf.value;
                dstNode.rotationKeyframes.push_back(dkf);
            }

            // scaleキーフレーム
            dstNode.scaleKeyframes.reserve(
                srcNode.scaleKeyframes.size());
            for (const auto& kf : srcNode.scaleKeyframes)
            {
                Model::VectorKeyframe dkf;
                dkf.seconds = kf.seconds;
                dkf.value = kf.value;
                dstNode.scaleKeyframes.push_back(dkf);
            }

            dst.nodeAnims.push_back(std::move(dstNode));
        }

        model.animations.push_back(std::move(dst));
    }
}
