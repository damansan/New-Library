#include "GLTFImporter.h"

#define TINYGLTF_IMPLEMENTATION
#include "tiny_gltf.h"
#include "GpuResourceUtils.h"
#include "Utility/misc.h"
#include <filesystem>
#include <algorithm>

// 画像読み込み無効化
static bool NullLoadImageData(
	tinygltf::Image*, const int, std::string*,
	std::string*, int, int, const unsigned char*, int, void*)
{
	return true;
}


// モデル読み込み
std::shared_ptr<Model> GLTFImporter::Load(
	ID3D11Device* device,
	const std::string& filename)
{
	tinygltf::TinyGLTF loader;
	loader.SetImageLoader(NullLoadImageData, nullptr);

	tinygltf::Model gltfModel;
	std::string error, warning;
	bool succeeded = false;


	// glb読み込み
	if (filename.find(".glb") != std::string::npos)
	{
		succeeded = loader.LoadBinaryFromFile(
			&gltfModel, &error, &warning, filename.c_str());
	}
	// gltf読み込み
	else if (filename.find(".gltf") != std::string::npos)
	{
		succeeded = loader.LoadASCIIFromFile(
			&gltfModel, &error, &warning, filename.c_str());
	}

	_ASSERT_EXPR_A(warning.empty(), warning.c_str());
	_ASSERT_EXPR_A(error.empty(), error.c_str());
	_ASSERT_EXPR_A(succeeded, "Failed to load glTF file");

	auto model = std::make_shared<Model>();
	model->hasSkin = CheckHasSkin(gltfModel);

	std::filesystem::path filepath(filename);

	// 読み込み順序
	// ノード
	LoadNodes(*model, gltfModel);
	// メッシュ）
	LoadMeshes(*model, gltfModel, device);
	// マテリアル
	LoadMaterials(*model, gltfModel, device, filepath);
	// スキン
	LoadSkins(*model, gltfModel);
	// アニメーション
	LoadAnimations(*model, gltfModel);

	// スキン完了後にbones.offsetTransformを補完
	for (auto& mesh : model->meshes)
	{
		if (mesh.skinIndex < 0) continue;
		const auto& skin = model->skins[mesh.skinIndex];
		for (size_t ji = 0; ji < mesh.bones.size(); ji++)
		{
			if (ji < skin.inverseBindMatrices.size())
			{
				mesh.bones[ji].offsetTransform =
					skin.inverseBindMatrices[ji];
			}
		}
	}

	return model;
}

// グローバルトランスフォーム計算
void GLTFImporter::CalcGlobalTransform(
	std::vector<Model::Node>& nodes,
	int nodeIndex,
	const DirectX::XMFLOAT4X4& parentTransform)
{
	auto& node = nodes[nodeIndex];

	// Scale行列
	DirectX::XMMATRIX S = DirectX::XMMatrixScaling(
		node.scale.x, node.scale.y, node.scale.z);

	// クォータニオンを正規化してwが正になるよう統一
	DirectX::XMVECTOR q = DirectX::XMVectorSet(
		node.rotation.x, node.rotation.y,
		node.rotation.z, node.rotation.w);
	q = DirectX::XMQuaternionNormalize(q);

	// wが負なら全成分を反転（同じ回転を表す）
	float w;
	DirectX::XMStoreFloat(&w, DirectX::XMVectorSplatW(q));
	if (w < 0.0f) q = DirectX::XMVectorNegate(q);

	// Rotation行列
	//DirectX::XMMATRIX R = DirectX::XMMatrixRotationQuaternion(
	//	DirectX::XMVectorSet(
	//		node.rotation.x, node.rotation.y,
	//		node.rotation.z, node.rotation.w));
	DirectX::XMMATRIX R = DirectX::XMMatrixRotationQuaternion(q);

	// Translation行列
	DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(
		node.position.x, node.position.y, node.position.z);

	// ローカル行列 = S * R * T
	DirectX::XMMATRIX local = S * R * T;
	DirectX::XMMATRIX parent = DirectX::XMLoadFloat4x4(&parentTransform);

	// グローバル行列 = ローカル * 親
	DirectX::XMMATRIX global = local * parent;
	DirectX::XMStoreFloat4x4(&node.globalTransform, global);

	// 子ノードへ再帰
	for (int childIndex : node.childIndices)
	{
		CalcGlobalTransform(nodes, childIndex, node.globalTransform);
	}
}

// ノード読み込み
void GLTFImporter::LoadNodes(
	Model& model,
	const tinygltf::Model& gltfModel)
{
	model.nodes.reserve(gltfModel.nodes.size());

	for (const auto& gltfNode : gltfModel.nodes)
	{
		Model::Node node;
		node.name = gltfNode.name;
		node.childIndices = gltfNode.children;

		if (!gltfNode.matrix.empty())
		{
			// matrix形式 → 座標系変換してからSRTへ分解
			DirectX::XMFLOAT4X4 matrix =
				GLTFConverter::Matrix(gltfNode.matrix);

			DirectX::XMVECTOR S, R, T;
			DirectX::XMMatrixDecompose(
				&S, &R, &T,
				DirectX::XMLoadFloat4x4(&matrix));

			DirectX::XMStoreFloat3(&node.scale, S);
			DirectX::XMStoreFloat4(&node.rotation, R);
			DirectX::XMStoreFloat3(&node.position, T);
		}
		else
		{
			// TRS形式 → 各成分を座標系変換
			if (!gltfNode.translation.empty())
			{
				// 平行移動: Z反転
				node.position = GLTFConverter::Translation(
					gltfNode.translation[0],
					gltfNode.translation[1],
					gltfNode.translation[2]);
			}
			if (!gltfNode.rotation.empty())
			{
				// 回転: X,Y反転
				node.rotation = GLTFConverter::Rotation(
					gltfNode.rotation[0],
					gltfNode.rotation[1],
					gltfNode.rotation[2],
					gltfNode.rotation[3]);
			}
			if (!gltfNode.scale.empty())
			{
				// スケール: 変換なし
				node.scale = GLTFConverter::Scale(
					gltfNode.scale[0],
					gltfNode.scale[1],
					gltfNode.scale[2]);
			}
		}
		model.nodes.push_back(node);
	}

	// 親Index設定
	for (int i = 0; i < (int)model.nodes.size(); i++)
	{
		for (int child : model.nodes[i].childIndices)
		{
			model.nodes[child].parentIndex = i;
		}
	}

	// グローバルトランスフォーム計算
	DirectX::XMFLOAT4X4 identity =
	{ 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
	for (const auto& scene : gltfModel.scenes)
	{
		for (int rootIndex : scene.nodes)
		{
			CalcGlobalTransform(model.nodes, rootIndex, identity);
		}
	}
}

// メッシュ読み込み
void GLTFImporter::LoadMeshes(
	Model& model,
	const tinygltf::Model& gltfModel,
	ID3D11Device* device)
{
	for (const auto& gltfMesh : gltfModel.meshes)
	{
		for (const auto& primitive : gltfMesh.primitives)
		{
			Model::Mesh mesh;
			mesh.materialIndex = primitive.material;

			// Position読み込み + Z反転
			if (primitive.attributes.count("POSITION"))
			{
				const auto& acc = gltfModel.accessors[primitive.attributes.at("POSITION")];
				const auto& bv = gltfModel.bufferViews[acc.bufferView];
				const auto& buf = gltfModel.buffers[bv.buffer];
				const float* data = reinterpret_cast<const float*>(
					buf.data.data() + bv.byteOffset + acc.byteOffset);

				mesh.vertices.resize(acc.count);
				for (size_t i = 0; i < acc.count; i++)
				{
					// 右手系→左手系: Z反転
					mesh.vertices[i].position = GLTFConverter::Position(
						data[i * 3 + 0],
						data[i * 3 + 1],
						data[i * 3 + 2]);
				}
			}

			// Normal読み込み + Z反転
			if (primitive.attributes.count("NORMAL"))
			{
				const auto& acc = gltfModel.accessors[primitive.attributes.at("NORMAL")];
				const auto& bv = gltfModel.bufferViews[acc.bufferView];
				const auto& buf = gltfModel.buffers[bv.buffer];
				const float* data = reinterpret_cast<const float*>(
					buf.data.data() + bv.byteOffset + acc.byteOffset);

				for (size_t i = 0; i < acc.count; i++)
				{
					// 右手系→左手系: Z反転
					mesh.vertices[i].normal = GLTFConverter::Normal(
						data[i * 3 + 0],
						data[i * 3 + 1],
						data[i * 3 + 2]);
				}
			}

			// Texcoord読み込み（変換なし）
			if (primitive.attributes.count("TEXCOORD_0"))
			{
				const auto& acc = gltfModel.accessors[primitive.attributes.at("TEXCOORD_0")];
				const auto& bv = gltfModel.bufferViews[acc.bufferView];
				const auto& buf = gltfModel.buffers[bv.buffer];
				const float* data = reinterpret_cast<const float*>(
					buf.data.data() + bv.byteOffset + acc.byteOffset);

				for (size_t i = 0; i < acc.count; i++)
				{
					mesh.vertices[i].texcoord =
					{
						data[i * 2 + 0],
						data[i * 2 + 1]
					};
				}
			}

			// BoneWeight読み込み（変換なし）
			if (primitive.attributes.count("WEIGHTS_0"))
			{
				const auto& acc = gltfModel.accessors[primitive.attributes.at("WEIGHTS_0")];
				const auto& bv = gltfModel.bufferViews[acc.bufferView];
				const auto& buf = gltfModel.buffers[bv.buffer];
				const float* data = reinterpret_cast<const float*>(
					buf.data.data() + bv.byteOffset + acc.byteOffset);

				for (size_t i = 0; i < acc.count; i++)
				{
					mesh.vertices[i].boneWeight =
					{
						data[i * 4 + 0],
						data[i * 4 + 1],
						data[i * 4 + 2],
						data[i * 4 + 3]
					};
				}
			}

			// BoneIndex読み込み（変換なし）
			if (primitive.attributes.count("JOINTS_0"))
			{
				const auto& acc = gltfModel.accessors[primitive.attributes.at("JOINTS_0")];
				const auto& bv = gltfModel.bufferViews[acc.bufferView];
				const auto& buf = gltfModel.buffers[bv.buffer];
				const uint16_t* data = reinterpret_cast<const uint16_t*>(
					buf.data.data() + bv.byteOffset + acc.byteOffset);

				for (size_t i = 0; i < acc.count; i++)
				{
					mesh.vertices[i].boneIndex =
					{
						data[i * 4 + 0],
						data[i * 4 + 1],
						data[i * 4 + 2],
						data[i * 4 + 3]
					};
				}
			}

			// Index読み込み
			if (primitive.indices >= 0)
			{
				const auto& acc = gltfModel.accessors[primitive.indices];
				const auto& bv = gltfModel.bufferViews[acc.bufferView];
				const auto& buf = gltfModel.buffers[bv.buffer];
				const void* data = buf.data.data() +
					bv.byteOffset + acc.byteOffset;

				mesh.indices.resize(acc.count);

				if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
				{
					const uint16_t* src = static_cast<const uint16_t*>(data);
					for (size_t i = 0; i < acc.count; i++)
					{
						mesh.indices[i] = src[i];
					}
				}
				else if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
				{
					const uint32_t* src = static_cast<const uint32_t*>(data);
					for (size_t i = 0; i < acc.count; i++)
					{
						mesh.indices[i] = src[i];
					}
				}
			}

			// ボーン情報読み込み
			if (primitive.attributes.count("JOINTS_0"))
			{
				int meshIndex = (int)(&gltfMesh - gltfModel.meshes.data());

				for (int ni = 0; ni < (int)gltfModel.nodes.size(); ni++)
				{
					const auto& gltfNode = gltfModel.nodes[ni];
					if (gltfNode.mesh != meshIndex || gltfNode.skin < 0)
					{
						continue;
					}

					const auto& skin = gltfModel.skins[gltfNode.skin];
					mesh.bones.resize(skin.joints.size());
					for (size_t ji = 0; ji < skin.joints.size(); ji++)
					{
						mesh.bones[ji].nodeIndex = skin.joints[ji];
						mesh.bones[ji].offsetTransform =
						{
							1,0,0,0,
							0,1,0,0,
							0,0,1,0,
							0,0,0,1
						};
					}
					break;
				}
			}

			// タンジェント計算
			ComputeTangents(mesh.vertices, mesh.indices);

			// 頂点バッファ生成
			{
				D3D11_BUFFER_DESC desc{};
				desc.ByteWidth = static_cast<UINT>(sizeof(Model::Vertex) * mesh.vertices.size());
				desc.Usage = D3D11_USAGE_DEFAULT;
				desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

				D3D11_SUBRESOURCE_DATA sub{};
				sub.pSysMem = mesh.vertices.data();
				device->CreateBuffer(&desc, &sub, mesh.vertexBuffer.GetAddressOf());
			}
			// インデックスバッファ生成
			{
				D3D11_BUFFER_DESC desc{};
				desc.ByteWidth = static_cast<UINT>(sizeof(uint32_t) * mesh.indices.size());
				desc.Usage = D3D11_USAGE_DEFAULT;
				desc.BindFlags = D3D11_BIND_INDEX_BUFFER;

				D3D11_SUBRESOURCE_DATA sub{};
				sub.pSysMem = mesh.indices.data();
				device->CreateBuffer(&desc, &sub, mesh.indexBuffer.GetAddressOf());
			}
			model.meshes.push_back(std::move(mesh));
		}
	}
}

// マテリアル読み込み
void GLTFImporter::LoadMaterials(
	Model& model,
	const tinygltf::Model& gltfModel,
	ID3D11Device* device,
	const std::filesystem::path& filepath)
{
	for (const auto& gltfMaterial : gltfModel.materials)
	{
		Model::Material material;
		material.name = gltfMaterial.name;

		const auto& pbr = gltfMaterial.pbrMetallicRoughness;

		if (!pbr.baseColorFactor.empty())
		{
			material.baseColor =
			{
				static_cast<float>(pbr.baseColorFactor[0]),
				static_cast<float>(pbr.baseColorFactor[1]),
				static_cast<float>(pbr.baseColorFactor[2]),
				static_cast<float>(pbr.baseColorFactor[3])
			};
		}

		material.roughnessFactor = static_cast<float>(pbr.roughnessFactor);
		material.metallicFactor = static_cast<float>(pbr.metallicFactor);

		if (gltfMaterial.alphaMode == "BLEND")
		{
			material.alphaMode = Model::Material::AlphaMode::Blend;
		}
		else if (gltfMaterial.alphaMode == "MASK")
		{
			material.alphaMode = Model::Material::AlphaMode::Mask;
		}

		material.alphaCutoff = static_cast<float>(gltfMaterial.alphaCutoff);

		// テクスチャ読み込みヘルパー
		auto loadTexture = [&](
			int textureIndex,
			Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& srv,
			DWORD dummyColor)
			{
				if (textureIndex < 0)
				{
					GpuResourceUtils::CreateDummyTexture(device, srv, dummyColor, 1);
					return;
				}

				const auto& texture = gltfModel.textures[textureIndex];
				const auto& image = gltfModel.images[texture.source];

				if (!image.uri.empty())
				{
					std::string texPath = (filepath.parent_path() / image.uri).string();
					GpuResourceUtils::LoadTexture(device, texPath, srv);
				}
				else if (image.bufferView >= 0)
				{
					const auto& bv = gltfModel.bufferViews[image.bufferView];
					const auto& buf = gltfModel.buffers[bv.buffer];
					const unsigned char* data = buf.data.data() + bv.byteOffset;
					GpuResourceUtils::LoadTexture(device, data, bv.byteLength, srv);
				}
				else
				{
					GpuResourceUtils::CreateDummyTexture(device, srv, dummyColor, 1);
				}
			};

		loadTexture(pbr.baseColorTexture.index,
			material.baseMap, 0xFFFFFFFF);
		loadTexture(gltfMaterial.normalTexture.index,
			material.normalMap, 0xFFFF8080);
		loadTexture(pbr.metallicRoughnessTexture.index,
			material.metallicRoughnessMap, 0xFFFFFFFF);
		loadTexture(gltfMaterial.occlusionTexture.index,
			material.occlusionMap, 0xFFFFFFFF);
		loadTexture(gltfMaterial.emissiveTexture.index,
			material.emissiveMap, 0xFF000000);

		model.materials.push_back(std::move(material));
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

// スキン読み込み
void GLTFImporter::LoadSkins(
	Model& model,
	const tinygltf::Model& gltfModel)
{
	model.skins.resize(gltfModel.skins.size());

	for (int si = 0; si < (int)gltfModel.skins.size(); si++)
	{
		const auto& gltfSkin = gltfModel.skins[si];
		auto& skin = model.skins[si];

		// joints配列（jointIndex → nodeIndex）
		skin.joints = gltfSkin.joints;

		// inverseBindMatrices読み込み + 座標系変換
		if (gltfSkin.inverseBindMatrices >= 0)
		{
			const auto& acc = gltfModel.accessors[
				gltfSkin.inverseBindMatrices];
			const auto& bv = gltfModel.bufferViews[acc.bufferView];
			const auto& buf = gltfModel.buffers[bv.buffer];
			const float* data = reinterpret_cast<const float*>(
				buf.data.data() + bv.byteOffset + acc.byteOffset);

			skin.inverseBindMatrices.resize(acc.count);
			for (size_t i = 0; i < acc.count; i++)
			{
				// glTF列優先 → DirectX行優先に転置 + Z反転
				const float* m = data + i * 16;
				//std::vector<double> md(m, m + 16);
				//skin.inverseBindMatrices[i] = GLTFConverter::Matrix(md);
				skin.inverseBindMatrices[i] = {
					m[0], m[4], m[8],  m[12],
					m[1], m[5], m[9],  m[13],
					m[2], m[6], m[10], m[14],
					m[3], m[7], m[11], m[15]
				};
			}
		}
	}

	// 各メッシュにskinIndexを設定
	for (int ni = 0; ni < (int)gltfModel.nodes.size(); ni++)
	{
		const auto& gltfNode = gltfModel.nodes[ni];
		if (gltfNode.mesh < 0 || gltfNode.skin < 0)
		{
			continue;
		}

		int primitiveOffset = 0;
		for (int mi = 0; mi < gltfNode.mesh; mi++)
		{
			primitiveOffset += (int)gltfModel.meshes[mi].primitives.size();
		}

		int primitiveCount = (int)gltfModel.meshes[gltfNode.mesh].primitives.size();
		for (int pi = 0; pi < primitiveCount; pi++)
		{
			model.meshes[primitiveOffset + pi].skinIndex = gltfNode.skin;
		}
	}
}

// アニメーション読み込み
void GLTFImporter::LoadAnimations(
	Model& model,
	const tinygltf::Model& gltfModel)
{
	for (const auto& gltfAnim : gltfModel.animations)
	{
		Model::Animation animation;
		animation.name = gltfAnim.name;
		animation.secondsLength = 0.0f;
		animation.nodeAnims.resize(model.nodes.size());

		for (const auto& channel : gltfAnim.channels)
		{
			const auto& sampler = gltfAnim.samplers[channel.sampler];
			const auto& inputAcc = gltfModel.accessors[sampler.input];
			const auto& outputAcc = gltfModel.accessors[sampler.output];
			const auto& inputBV = gltfModel.bufferViews[inputAcc.bufferView];
			const auto& outputBV = gltfModel.bufferViews[outputAcc.bufferView];
			const auto& inputBuf = gltfModel.buffers[inputBV.buffer];
			const auto& outputBuf = gltfModel.buffers[outputBV.buffer];

			const float* times = reinterpret_cast<const float*>(
				inputBuf.data.data() +
				inputBV.byteOffset + inputAcc.byteOffset);

			const float* values = reinterpret_cast<const float*>(
				outputBuf.data.data() +
				outputBV.byteOffset + outputAcc.byteOffset);

			int nodeIndex = channel.target_node;
			if (nodeIndex < 0 || nodeIndex >= (int)animation.nodeAnims.size())
			{
				continue;
			}

			auto& nodeAnim = animation.nodeAnims[nodeIndex];

			// アニメーション長更新
			if (inputAcc.count > 0)
			{
				animation.secondsLength = (std::max)(
					animation.secondsLength,
					times[inputAcc.count - 1]);
			}

			// Translation + Z反転
			if (channel.target_path == "translation")
			{
				for (size_t i = 0; i < inputAcc.count; i++)
				{
					Model::VectorKeyframe kf;
					kf.seconds = times[i];

					// 右手系→左手系: Z反転
					kf.value = GLTFConverter::AnimPosition(
						values[i * 3 + 0],
						values[i * 3 + 1],
						values[i * 3 + 2]);
					nodeAnim.positionKeyframes.push_back(kf);
				}
			}

			// Rotation + X,Y反転
			else if (channel.target_path == "rotation")
			{
				for (size_t i = 0; i < inputAcc.count; i++)
				{
					Model::QuaternionKeyframe kf;
					kf.seconds = times[i];
					// 右手系→左手系: X,Y反転
					kf.value = GLTFConverter::AnimRotation(
						values[i * 4 + 0],
						values[i * 4 + 1],
						values[i * 4 + 2],
						values[i * 4 + 3]);
					nodeAnim.rotationKeyframes.push_back(kf);
				}
			}

			// Scale（変換なし）
			else if (channel.target_path == "scale")
			{
				for (size_t i = 0; i < inputAcc.count; i++)
				{
					Model::VectorKeyframe kf;
					kf.seconds = times[i];
					kf.value = GLTFConverter::AnimScale(
						values[i * 3 + 0],
						values[i * 3 + 1],
						values[i * 3 + 2]);
					nodeAnim.scaleKeyframes.push_back(kf);
				}
			}
		}
		model.animations.push_back(std::move(animation));
	}
}

// スキニング判定
bool GLTFImporter::CheckHasSkin(const tinygltf::Model& gltfModel)
{
	return !gltfModel.skins.empty();
}

// タンジェント計算
void GLTFImporter::ComputeTangents(std::vector<Model::Vertex>& vertices, const std::vector<uint32_t>& indices)
{
	for (size_t i = 0; i < indices.size(); i += 3)
	{
		auto& v0 = vertices[indices[i + 0]];
		auto& v1 = vertices[indices[i + 1]];
		auto& v2 = vertices[indices[i + 2]];

		DirectX::XMVECTOR p0 = DirectX::XMLoadFloat3(&v0.position);
		DirectX::XMVECTOR p1 = DirectX::XMLoadFloat3(&v1.position);
		DirectX::XMVECTOR p2 = DirectX::XMLoadFloat3(&v2.position);

		DirectX::XMVECTOR uv0 = DirectX::XMLoadFloat2(&v0.texcoord);
		DirectX::XMVECTOR uv1 = DirectX::XMLoadFloat2(&v1.texcoord);
		DirectX::XMVECTOR uv2 = DirectX::XMLoadFloat2(&v2.texcoord);

		DirectX::XMVECTOR e1 =
			DirectX::XMVectorSubtract(p1, p0);
		DirectX::XMVECTOR e2 =
			DirectX::XMVectorSubtract(p2, p0);
		DirectX::XMVECTOR du1 =
			DirectX::XMVectorSubtract(uv1, uv0);
		DirectX::XMVECTOR du2 =
			DirectX::XMVectorSubtract(uv2, uv0);

		float u1 = DirectX::XMVectorGetX(du1);
		float v1f = DirectX::XMVectorGetY(du1);
		float u2 = DirectX::XMVectorGetX(du2);
		float v2f = DirectX::XMVectorGetY(du2);

		float denom = u1 * v2f - u2 * v1f;
		if (fabsf(denom) < 1e-6f)
		{
			continue;
		}

		float r = 1.0f / denom;

		DirectX::XMVECTOR tangent = DirectX::XMVector3Normalize(
			DirectX::XMVectorScale(
				DirectX::XMVectorSubtract(
					DirectX::XMVectorScale(e1, v2f),
					DirectX::XMVectorScale(e2, v1f)),
				r));

		DirectX::XMFLOAT3 t;
		DirectX::XMStoreFloat3(&t, tangent);

		v0.tangent = v1.tangent = v2.tangent =
		{ t.x, t.y, t.z, 1.0f };
	}
}
