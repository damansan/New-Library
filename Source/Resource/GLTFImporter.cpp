#include "GLTFImporter.h"

#define TINYGLTF_IMPLEMENTATION
#include "tiny_gltf.h"
#include "GpuResourceUtils.h"
#include "Utility/misc.h"
#include <stack>
#include <filesystem>
#include <algorithm>


// 画像読み込み無効化
static bool NullLoadImageData(
	tinygltf::Image*, const int, std::string*,
	std::string*, int, int, const unsigned char*, int, void*)
{
	return true;
}


// モデル読み込み＆スキニング判定
std::shared_ptr<Model> GLTFImporter::Load(
	ID3D11Device* device,
	const std::string& filename)
{
	// GLTF読み込み用ローダー作成
	tinygltf::TinyGLTF loader;

	// 画像読み込み無効化
	loader.SetImageLoader(NullLoadImageData, nullptr);

	// GLTFデータ本体
	tinygltf::Model gltfModel;
	// エラー/警告メッセージ用
	std::string error, warning;
	// 読み込み成功フラグ
	bool succeeded = false;

	// glb読み込み
	if (filename.find(".glb") != std::string::npos)
	{
		succeeded = loader.LoadBinaryFromFile(&gltfModel, &error, &warning, filename.c_str());
	}
	// gltf読み込み
	else if (filename.find(".gltf") != std::string::npos)
	{
		succeeded = loader.LoadASCIIFromFile(&gltfModel, &error, &warning, filename.c_str());
	}

	// 警告
	_ASSERT_EXPR_A(warning.empty(), warning.c_str());
	// エラー
	_ASSERT_EXPR_A(error.empty(), error.c_str());
	// 失敗
	_ASSERT_EXPR_A(succeeded, "Failed to load glTF file");

	// Model生成
	auto model = std::make_shared<Model>();

	// スキニング自動判定
	model->hasSkin = CheckHasSkin(gltfModel);

	// ファイルパス取得
	std::filesystem::path filepath(filename);

	// ノード読み込み
	LoadNodes(*model, gltfModel);
	// メッシュ読み込み
	LoadMeshes(*model, gltfModel, device);
	// マテリアル読み込み
	LoadMaterials(*model, gltfModel, device, filepath);
	// アニメーション読み込み
	LoadAnimations(*model, gltfModel);

	return model;
}

// ノード読み込み
void GLTFImporter::LoadNodes(
	Model& model,
	const tinygltf::Model& gltfModel)
{
	// ノード数分メモリ確保
	model.nodes.reserve(gltfModel.nodes.size());

	// gltf内全ノード走査　
	for (const auto& gltfNode : gltfModel.nodes)
	{
		Model::Node node;
		// ノード名
		node.name = gltfNode.name;

		// 子ノードIndex保存
		node.childIndices = gltfNode.children;

		// トランスフォーム読み込み
		if (!gltfNode.matrix.empty())
		{
			// matrix形式の場合
			DirectX::XMFLOAT4X4 matrix = ToXMFLOAT4X4(gltfNode.matrix);
			// 行列をSRTへ分解
			DirectX::XMVECTOR S, R, T;
			DirectX::XMMatrixDecompose(
				&S, &R, &T,
				DirectX::XMLoadFloat4x4(&matrix));
			// Scale保存
			DirectX::XMStoreFloat3(&node.scale, S);
			// Rotation保存
			DirectX::XMStoreFloat4(&node.rotation, R);
			// Translation保存
			DirectX::XMStoreFloat3(&node.position, T);
		}
		else
		{
			// TRS形式の場合

			// translation
			if (!gltfNode.translation.empty())
				node.position = ToXMFLOAT3(gltfNode.translation);
			// rotation
			if (!gltfNode.rotation.empty())
				node.rotation = ToXMFLOAT4(gltfNode.rotation);
			// scale
			if (!gltfNode.scale.empty())
				node.scale = ToXMFLOAT3(gltfNode.scale);
		}

		// ノード追加
		model.nodes.push_back(node);
	}

	// 親Index設定
	// 子ノード側に親Indexを書き込む
	for (int i = 0; i < static_cast<int>(model.nodes.size()); i++)
	{
		for (int child : model.nodes[i].childIndices)
		{
			model.nodes[child].parentIndex = i;
		}
	}

	// グローバルトランスフォーム計算
	DirectX::XMFLOAT4X4 identity = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
	for (const auto& scene : gltfModel.scenes)
	{
		for (int rootIndex : scene.nodes)
		{
			// 親から子へTransform伝搬
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
	// glTF内全メッシュ走査
	for (const auto& gltfMesh : gltfModel.meshes)
	{
		// Primitive単位でメッシュ生成(1メッシュに複数Primitiveを持てる）
		for (const auto& primitive : gltfMesh.primitives)
		{
			Model::Mesh mesh;
			// マテリアルIndex
			mesh.materialIndex = primitive.material;

			// Position読み込み
			if (primitive.attributes.count("POSITION"))
			{
				const auto& accessor = gltfModel.accessors[primitive.attributes.at("POSITION")];
				const auto& bufferView = gltfModel.bufferViews[accessor.bufferView];
				const auto& buffer = gltfModel.buffers[bufferView.buffer];

				// 頂点座標データ取得
				const float* data = reinterpret_cast<const float*>(
					buffer.data.data() + bufferView.byteOffset + accessor.byteOffset);

				// 頂点数保存
				mesh.vertices.resize(accessor.count);

				// xyz読み込み
				for (size_t i = 0; i < accessor.count; i++)
				{
					mesh.vertices[i].position =
					{
						data[i * 3 + 0],
						data[i * 3 + 1],
						data[i * 3 + 2]
					};
				}
			}

			// Normal読み込み
			if (primitive.attributes.count("NORMAL"))
			{
				const auto& accessor = gltfModel.accessors[primitive.attributes.at("NORMAL")];
				const auto& bufferView = gltfModel.bufferViews[accessor.bufferView];
				const auto& buffer = gltfModel.buffers[bufferView.buffer];

				// 頂点データ取得
				const float* data = reinterpret_cast<const float*>(
					buffer.data.data() + bufferView.byteOffset + accessor.byteOffset);

				// 法線ベクトル読み込み
				for (size_t i = 0; i < accessor.count; i++)
				{
					mesh.vertices[i].normal =
					{
						data[i * 3 + 0],
						data[i * 3 + 1],
						data[i * 3 + 2]
					};
				}
			}

			// Texcoord読み込み
			if (primitive.attributes.count("TEXCOORD_0"))
			{
				const auto& accessor = gltfModel.accessors[primitive.attributes.at("TEXCOORD_0")];
				const auto& bufferView = gltfModel.bufferViews[accessor.bufferView];
				const auto& buffer = gltfModel.buffers[bufferView.buffer];

				// 頂点データ取得
				const float* data = reinterpret_cast<const float*>(
					buffer.data.data() + bufferView.byteOffset + accessor.byteOffset);

				// UV座標読み込み
				for (size_t i = 0; i < accessor.count; i++)
				{
					mesh.vertices[i].texcoord =
					{
						data[i * 2 + 0],
						data[i * 2 + 1]
					};
				}
			}

			// BoneWeight読み込み
			if (primitive.attributes.count("WEIGHTS_0"))
			{
				const auto& accessor = gltfModel.accessors[primitive.attributes.at("WEIGHTS_0")];
				const auto& bufferView = gltfModel.bufferViews[accessor.bufferView];
				const auto& buffer = gltfModel.buffers[bufferView.buffer];

				// 頂点データ取得
				const float* data = reinterpret_cast<const float*>(
					buffer.data.data() + bufferView.byteOffset + accessor.byteOffset);

				for (size_t i = 0; i < accessor.count; i++)
				{
					// ボーンウェイト読み込み
					mesh.vertices[i].boneWeight =
					{
						data[i * 4 + 0],
						data[i * 4 + 1],
						data[i * 4 + 2],
						data[i * 4 + 3]
					};
				}
			}

			// BoneIndex読み込み
			if (primitive.attributes.count("JOINTS_0"))
			{
				const auto& accessor = gltfModel.accessors[primitive.attributes.at("JOINTS_0")];
				const auto& bufferView = gltfModel.bufferViews[accessor.bufferView];
				const auto& buffer = gltfModel.buffers[bufferView.buffer];

				// 頂点データ取得
				const uint16_t* data = reinterpret_cast<const uint16_t*>(
					buffer.data.data() + bufferView.byteOffset + accessor.byteOffset);

				// ボーンIndex読み込み
				for (size_t i = 0; i < accessor.count; i++)
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
				const auto& accessor = gltfModel.accessors[primitive.indices];
				const auto& bufferView = gltfModel.bufferViews[accessor.bufferView];
				const auto& buffer = gltfModel.buffers[bufferView.buffer];

				// Indexデータ先頭
				const void* data = buffer.data.data() +
					bufferView.byteOffset + accessor.byteOffset;

				mesh.indices.resize(accessor.count);

				// uint16形式
				if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
				{
					const uint16_t* src = static_cast<const uint16_t*>(data);
					for (size_t i = 0; i < accessor.count; i++)
					{
						mesh.indices[i] = src[i];
					}
				}
				// uint32形式
				else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
				{
					const uint32_t* src = static_cast<const uint32_t*>(data);
					for (size_t i = 0; i < accessor.count; i++)
					{
						mesh.indices[i] = src[i];
					}
				}
			}

			// タンジェント計算
			ComputeTangents(mesh.vertices, mesh.indices);

			// 頂点バッファ生成
			{
				D3D11_BUFFER_DESC desc{};
				desc.ByteWidth = static_cast<UINT>(
					sizeof(Model::Vertex) * mesh.vertices.size());
				desc.Usage = D3D11_USAGE_DEFAULT;
				desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

				D3D11_SUBRESOURCE_DATA sub{};
				sub.pSysMem = mesh.vertices.data();
				device->CreateBuffer(&desc, &sub,
					mesh.vertexBuffer.GetAddressOf());
			}

			// インデックスバッファ生成
			{
				D3D11_BUFFER_DESC desc{};
				desc.ByteWidth = static_cast<UINT>(
					sizeof(uint32_t) * mesh.indices.size());
				desc.Usage = D3D11_USAGE_DEFAULT;
				desc.BindFlags = D3D11_BIND_INDEX_BUFFER;

				D3D11_SUBRESOURCE_DATA sub{};
				sub.pSysMem = mesh.indices.data();
				device->CreateBuffer(&desc, &sub,
					mesh.indexBuffer.GetAddressOf());
			}

			// モデルへ追加
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
	// glTF内全マテリアル走査
	for (const auto& gltfMaterial : gltfModel.materials)
	{
		Model::Material material;
		// マテリアル名
		material.name = gltfMaterial.name;

		// PBR設定取得
		const auto& pbr = gltfMaterial.pbrMetallicRoughness;
		// BaseColor読み込み
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
		// Roughness値
		material.roughnessFactor = static_cast<float>(pbr.roughnessFactor);
		// Metallic値
		material.metallicFactor = static_cast<float>(pbr.metallicFactor);

		// AlphaBlend
		if (gltfMaterial.alphaMode == "BLEND")
		{
			material.alphaMode = Model::Material::AlphaMode::Blend;
		}
		// AlphaMask
		else if (gltfMaterial.alphaMode == "MASK")
		{
			material.alphaMode = Model::Material::AlphaMode::Mask;
		}
		// AlphaCutoff値
		material.alphaCutoff = static_cast<float>(gltfMaterial.alphaCutoff);

		// テクスチャ読み込みヘルパー
		auto loadTexture = [&](int textureIndex,
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
					// 外部ファイルテクスチャ
					std::string texPath = (filepath.parent_path() / image.uri).string();
					GpuResourceUtils::LoadTexture(device, texPath, srv);
				}
				else if (image.bufferView >= 0)
				{
					// 埋め込みテクスチャ（GLBファイル）
					// bufferViewから直接データを取得する
					const auto& bv = gltfModel.bufferViews[image.bufferView];
					const auto& buffer = gltfModel.buffers[bv.buffer];
					const unsigned char* data = buffer.data.data() + bv.byteOffset;

					GpuResourceUtils::LoadTexture(device, data, bv.byteLength, srv);
				}
				else
				{
					// どちらもない場合はダミー
					GpuResourceUtils::CreateDummyTexture(device, srv, dummyColor, 1);
				}
			};

		// ベースカラー読み込み
		loadTexture(pbr.baseColorTexture.index,
			material.baseMap, 0xFFFFFFFF);
		// ノーマルマップ読み込み
		loadTexture(gltfMaterial.normalTexture.index,
			material.normalMap, 0xFFFF8080);
		// メタリック/ラフネス読み込み
		loadTexture(pbr.metallicRoughnessTexture.index,
			material.metallicRoughnessMap, 0xFFFFFFFF);
		// オクルージョンマップ読み込み
		loadTexture(gltfMaterial.occlusionTexture.index,
			material.occlusionMap, 0xFFFFFFFF);
		// エミッシブ読み込み
		loadTexture(gltfMaterial.emissiveTexture.index,
			material.emissiveMap, 0xFF000000);

		// モデルへ追加
		model.materials.push_back(std::move(material));
	}

	// メッシュとマテリアルを紐付け
	for (auto& mesh : model.meshes)
	{
		// 有効範囲チェック
		if (mesh.materialIndex >= 0 && mesh.materialIndex < static_cast<int>(model.materials.size()))
		{
			// マテリアル設定
			mesh.material = &model.materials[mesh.materialIndex];
		}
	}
}

// アニメーション読み込み
void GLTFImporter::LoadAnimations(
	Model& model,
	const tinygltf::Model& gltfModel,
	float sampleRate)
{
	// glTF内全アニメーション走査
	for (const auto& gltfAnim : gltfModel.animations)
	{
		Model::Animation animation;
		// アニメーション名
		animation.name = gltfAnim.name;
		// アニメーション長初期化
		animation.secondsLength = 0.0f;
		// ノード数分アニメーション確保
		animation.nodeAnims.resize(model.nodes.size());

		// Channel読み込み
		for (const auto& channel : gltfAnim.channels)
		{
			// Sampler取得
			const auto& sampler = gltfAnim.samplers[channel.sampler];
			// 入力Accessor（時間）
			const auto& inputAcc = gltfModel.accessors[sampler.input];
			// 出力Accessor（値）
			const auto& outputAcc = gltfModel.accessors[sampler.output];

			// BufferView取得
			const auto& inputBV = gltfModel.bufferViews[inputAcc.bufferView];
			const auto& outputBV = gltfModel.bufferViews[outputAcc.bufferView];
			// Buffer取得
			const auto& inputBuf = gltfModel.buffers[inputBV.buffer];
			const auto& outputBuf = gltfModel.buffers[outputBV.buffer];

			// キーフレーム時間配列
			const float* times = reinterpret_cast<const float*>(
				inputBuf.data.data() + inputBV.byteOffset + inputAcc.byteOffset);
			// キーフレーム値配列
			const float* values = reinterpret_cast<const float*>(
				outputBuf.data.data() + outputBV.byteOffset + outputAcc.byteOffset);

			// 対象ノードIndex
			int nodeIndex = channel.target_node;
			// 範囲外チェック
			if (nodeIndex < 0 || nodeIndex >= static_cast<int>(animation.nodeAnims.size()))
			{
				continue;
			}

			// ノードアニメーション取得
			auto& nodeAnim = animation.nodeAnims[nodeIndex];

			// アニメーション長更新
			if (inputAcc.count > 0)
			{
				// 最終キーフレーム時間
				float lastTime = times[inputAcc.count - 1];

				// 最大時間更新
				animation.secondsLength = (std::max)(animation.secondsLength, lastTime);
			}


			// Translation
			if (channel.target_path == "translation")
			{
				for (size_t i = 0; i < inputAcc.count; i++)
				{
					Model::VectorKeyframe kf;
					// キーフレーム時間
					kf.seconds = times[i];
					// 座標値
					kf.value =
					{
						values[i * 3 + 0],
						values[i * 3 + 1],
						values[i * 3 + 2]
					};

					nodeAnim.positionKeyframes.push_back(kf);
				}
			}
			// Rotation
			else if (channel.target_path == "rotation")
			{
				for (size_t i = 0; i < inputAcc.count; i++)
				{
					Model::QuaternionKeyframe kf;

					// キーフレーム時間
					kf.seconds = times[i];
					// 回転値
					kf.value =
					{
						values[i * 4 + 0], values[i * 4 + 1],
						values[i * 4 + 2], values[i * 4 + 3]
					};
					nodeAnim.rotationKeyframes.push_back(kf);
				}
			}
			// Scale
			else if (channel.target_path == "scale")
			{
				for (size_t i = 0; i < inputAcc.count; i++)
				{
					Model::VectorKeyframe kf;
					// キーフレーム時間
					kf.seconds = times[i];
					// Scale値
					kf.value =
					{
						values[i * 3 + 0],
						values[i * 3 + 1],
						values[i * 3 + 2]
					};

					nodeAnim.scaleKeyframes.push_back(kf);
				}
			}
		}

		// モデルへ追加
		model.animations.push_back(std::move(animation));
	}
}

// スキニング判定
bool GLTFImporter::CheckHasSkin(const tinygltf::Model& gltfModel)
{
	return !gltfModel.skins.empty();
}


// 座標系変換ヘルパー
DirectX::XMFLOAT3 GLTFImporter::ToXMFLOAT3(const std::vector<double>& v)
{
	return { 
		static_cast<float>(v[0]),
		static_cast<float>(v[1]),
		static_cast<float>(v[2]) 
	};
}
DirectX::XMFLOAT4 GLTFImporter::ToXMFLOAT4(const std::vector<double>& v)
{
	return { 
		static_cast<float>(v[0]),
		static_cast<float>(v[1]),
		static_cast<float>(v[2]),
		static_cast<float>(v[3]) 
	};
}
DirectX::XMFLOAT4X4 GLTFImporter::ToXMFLOAT4X4(const std::vector<double>& v)
{
	return {
		static_cast<float>(v[0]),  static_cast<float>(v[1]),
		static_cast<float>(v[2]),  static_cast<float>(v[3]),
		static_cast<float>(v[4]),  static_cast<float>(v[5]),
		static_cast<float>(v[6]),  static_cast<float>(v[7]),
		static_cast<float>(v[8]),  static_cast<float>(v[9]),
		static_cast<float>(v[10]), static_cast<float>(v[11]),
		static_cast<float>(v[12]), static_cast<float>(v[13]),
		static_cast<float>(v[14]), static_cast<float>(v[15])
	};
}


// タンジェント計算
void GLTFImporter::ComputeTangents(
	std::vector<Model::Vertex>& vertices,
	const std::vector<uint32_t>& indices)
{
	// 3Index単位で三角形走査
	for (size_t i = 0; i < indices.size(); i += 3)
	{
		// 三角形頂点取得
		auto& v0 = vertices[indices[i + 0]];
		auto& v1 = vertices[indices[i + 1]];
		auto& v2 = vertices[indices[i + 2]];

		// Position取得
		DirectX::XMVECTOR p0 = DirectX::XMLoadFloat3(&v0.position);
		DirectX::XMVECTOR p1 = DirectX::XMLoadFloat3(&v1.position);
		DirectX::XMVECTOR p2 = DirectX::XMLoadFloat3(&v2.position);

		// UV取得
		DirectX::XMVECTOR uv0 = DirectX::XMLoadFloat2(&v0.texcoord);
		DirectX::XMVECTOR uv1 = DirectX::XMLoadFloat2(&v1.texcoord);
		DirectX::XMVECTOR uv2 = DirectX::XMLoadFloat2(&v2.texcoord);

		// 辺ベクトル計算
		DirectX::XMVECTOR e1 = DirectX::XMVectorSubtract(p1, p0);    // Position差分
		DirectX::XMVECTOR e2 = DirectX::XMVectorSubtract(p2, p0);
		DirectX::XMVECTOR du1 = DirectX::XMVectorSubtract(uv1, uv0); // UV差分
		DirectX::XMVECTOR du2 = DirectX::XMVectorSubtract(uv2, uv0);

		// UV成分取得
		float u1 = DirectX::XMVectorGetX(du1);
		float v1f = DirectX::XMVectorGetY(du1);
		float u2 = DirectX::XMVectorGetX(du2);
		float v2f = DirectX::XMVectorGetY(du2);

		// タンジェント計算係数
		float denom = u1 * v2f - u2 * v1f;
		// UVが壊れている場合
		if (fabsf(denom) < 1e-6f)
		{
			continue;
		}

		float r = 1.0f / denom;

		// タンジェント計算
		DirectX::XMVECTOR tangent = DirectX::XMVectorScale(
			DirectX::XMVectorSubtract(
				DirectX::XMVectorScale(e1, v2f),
				DirectX::XMVectorScale(e2, v1f)),
			r);
		// 正規化
		tangent = DirectX::XMVector3Normalize(tangent);

		// 頂点へ保存
		DirectX::XMFLOAT3 t;
		DirectX::XMStoreFloat3(&t, tangent);
		v0.tangent =
		v1.tangent =
		v2.tangent =
		{
			t.x,
			t.y,
			t.z,
			1.0f
		};
	}
}

// グローバルトランスフォーム計算
void GLTFImporter::CalcGlobalTransform(
	std::vector<Model::Node>& nodes,
	int nodeIndex,
	const DirectX::XMFLOAT4X4& parentTransform)
{
	// 対象ノード取得
	auto& node = nodes[nodeIndex];

	// Scale行列
	DirectX::XMMATRIX S = DirectX::XMMatrixScaling(
		node.scale.x, node.scale.y, node.scale.z);
	// Rotation行列
	DirectX::XMMATRIX R = DirectX::XMMatrixRotationQuaternion(
		DirectX::XMVectorSet(
			node.rotation.x, node.rotation.y,
			node.rotation.z, node.rotation.w));
	// Translation行列
	DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(
		node.position.x, node.position.y, node.position.z);

	// ローカル行列合成
	DirectX::XMMATRIX local = S * R * T;
	
	// 親行列取得
	DirectX::XMMATRIX parent = DirectX::XMLoadFloat4x4(&parentTransform);
	
	// グローバル行列計算
	DirectX::XMMATRIX global = local * parent;

	// 計算結果保存
	DirectX::XMStoreFloat4x4(&node.globalTransform, global);

	// 子ノード再帰処理
	for (int childIndex : node.childIndices)
	{
		// 親Transformを渡して再帰
		CalcGlobalTransform(nodes, childIndex, node.globalTransform);
	}
}
