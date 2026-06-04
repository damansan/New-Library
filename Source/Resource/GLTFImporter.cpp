#include "GLTFImporter.h"
#include "Utility/misc.h"
#include <filesystem>
#include <stack>
#include <algorithm>
#include <fstream>

#define TINYGLTF_IMPLEMENTATION
#include "tiny_gltf.h"


// 画像読み込み無効化
static bool NullLoadImageData(
	tinygltf::Image*, const int, std::string*,
	std::string*, int, int, const unsigned char*, int, void*)
{
	return true;
}

// GLTFファイルを読み込んでGLTFDataを返す
std::shared_ptr<GLTFData> GLTFImporter::Load(const std::string& filename)
{
	tinygltf::TinyGLTF loader;
	loader.SetImageLoader(NullLoadImageData, nullptr);

	tinygltf::Model gltfModel;
	std::string error, warning;
	bool succeeded = false;

	if (filename.find(".glb") != std::string::npos)
	{
		succeeded = loader.LoadBinaryFromFile(
			&gltfModel, &error, &warning, filename.c_str());
	}
	else if (filename.find(".gltf") != std::string::npos)
	{
		succeeded = loader.LoadASCIIFromFile(
			&gltfModel, &error, &warning, filename.c_str());
	}

	_ASSERT_EXPR_A(warning.empty(), warning.c_str());
	_ASSERT_EXPR_A(error.empty(), error.c_str());
	_ASSERT_EXPR_A(succeeded, "Failed to load glTF file");

	auto data = std::make_shared<GLTFData>();
	data->hasSkin = CheckHasSkin(gltfModel);

	std::filesystem::path filepath(filename);

	LoadNodes(*data, gltfModel);
	LoadTextures(*data, gltfModel, filepath);
	LoadMaterials(*data, gltfModel, filepath);
	LoadMeshes(*data, gltfModel);
	LoadAnimations(*data, gltfModel);

	return data;



	//// TODO デバッグ時間計測
	//// 計測用ヘルパー（ローカルラムダ）
	//auto now = []() {
	//	LARGE_INTEGER t;
	//	QueryPerformanceCounter(&t);
	//	return t.QuadPart;
	//	};
	//LARGE_INTEGER freq;
	//QueryPerformanceFrequency(&freq);
	//auto ms = [&](LONGLONG start, LONGLONG end) {
	//	return (double)(end - start) / freq.QuadPart * 1000.0;
	//	};

	//LONGLONG t0, t1;
	//char buf[256];

	//// ------- tinygltf ロード -------
	//t0 = now();
	//tinygltf::TinyGLTF loader;
	//loader.SetImageLoader(NullLoadImageData, nullptr);
	//tinygltf::Model gltfModel;
	//std::string error, warning;
	//bool succeeded = false;

	//if (filename.find(".glb") != std::string::npos)
	//	succeeded = loader.LoadBinaryFromFile(&gltfModel, &error, &warning, filename.c_str());
	//else if (filename.find(".gltf") != std::string::npos)
	//	succeeded = loader.LoadASCIIFromFile(&gltfModel, &error, &warning, filename.c_str());

	//t1 = now();
	//sprintf_s(buf, "[GLTF] TinyGLTF Load     : %.1f ms\n", ms(t0, t1));
	//OutputDebugStringA(buf);

	//_ASSERT_EXPR_A(warning.empty(), warning.c_str());
	//_ASSERT_EXPR_A(error.empty(), error.c_str());
	//_ASSERT_EXPR_A(succeeded, "Failed to load glTF file");

	//auto data = std::make_shared<GLTFData>();
	//data->hasSkin = CheckHasSkin(gltfModel);

	//std::filesystem::path filepath(filename);

	//// ------- LoadNodes -------
	//t0 = now();
	//LoadNodes(*data, gltfModel);
	//t1 = now();
	//sprintf_s(buf, "[GLTF] LoadNodes         : %.1f ms\n", ms(t0, t1));
	//OutputDebugStringA(buf);

	//// ------- LoadTextures -------
	//t0 = now();
	//LoadTextures(*data, gltfModel, filepath);
	//t1 = now();
	//sprintf_s(buf, "[GLTF] LoadTextures      : %.1f ms\n", ms(t0, t1));
	//OutputDebugStringA(buf);

	//// ------- LoadMaterials -------
	//t0 = now();
	//LoadMaterials(*data, gltfModel, filepath);
	//t1 = now();
	//sprintf_s(buf, "[GLTF] LoadMaterials     : %.1f ms\n", ms(t0, t1));
	//OutputDebugStringA(buf);

	//// ------- LoadMeshes -------
	//t0 = now();
	//LoadMeshes(*data, gltfModel);
	//t1 = now();
	//sprintf_s(buf, "[GLTF] LoadMeshes        : %.1f ms\n", ms(t0, t1));
	//OutputDebugStringA(buf);

	//// ------- LoadAnimations -------
	//t0 = now();
	//LoadAnimations(*data, gltfModel);
	//t1 = now();
	//sprintf_s(buf, "[GLTF] LoadAnimations    : %.1f ms\n", ms(t0, t1));
	//OutputDebugStringA(buf);

	//// ------- Convert -------
	//// ※ ResourceManager側で計測するか、ここには含まれないので別途必要なら追加

	//sprintf_s(buf, "[GLTF] nodes=%d meshes=%d textures=%d anims=%d\n",
	//	(int)data->nodes.size(),
	//	(int)data->meshes.size(),
	//	(int)data->textures.size(),
	//	(int)data->animations.size());
	//OutputDebugStringA(buf);

	//return data;
}

// ノード読み込み
void GLTFImporter::LoadNodes(
	GLTFData& data,
	const tinygltf::Model& gltfModel)
{
	data.nodes.reserve(gltfModel.nodes.size());

	for (const auto& gltfNode : gltfModel.nodes)
	{
		GLTFData::Node node;
		node.name = gltfNode.name;
		node.childIndices = gltfNode.children;

		if (!gltfNode.matrix.empty())
		{
			DirectX::XMFLOAT4X4 matrix = ToXMFLOAT4X4(gltfNode.matrix);
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
			if (!gltfNode.translation.empty())
				node.position = ToXMFLOAT3(gltfNode.translation);
			if (!gltfNode.rotation.empty())
				node.rotation = ToXMFLOAT4(gltfNode.rotation);
			if (!gltfNode.scale.empty())
				node.scale = ToXMFLOAT3(gltfNode.scale);
		}

		data.nodes.push_back(node);
	}

	// 親インデックス設定
	for (int i = 0; i < (int)data.nodes.size(); i++)
	{
		for (int child : data.nodes[i].childIndices)
		{
			data.nodes[child].parentIndex = i;
		}
	}

	// グローバルトランスフォーム計算
	DirectX::XMFLOAT4X4 identity = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
	for (int i = 0; i < (int)data.nodes.size(); i++)
	{
		if (data.nodes[i].parentIndex < 0)
		{
			CalcGlobalTransform(data.nodes, i, identity);
		}
	}
}

// メッシュ読み込み
void GLTFImporter::LoadMeshes(
	GLTFData& data,
	const tinygltf::Model& gltfModel)
{
	for (int meshIdx = 0; meshIdx < (int)gltfModel.meshes.size(); meshIdx++)
	{
		const auto& gltfMesh = gltfModel.meshes[meshIdx];

		for (const auto& primitive : gltfMesh.primitives)
		{
			GLTFData::MeshData mesh;

			// nodeIndex を逆引き
			for (int n = 0; n < (int)gltfModel.nodes.size(); n++)
			{
				if (gltfModel.nodes[n].mesh == meshIdx)
				{
					mesh.nodeIndex = n;
					break;
				}
			}

			mesh.materialIndex = primitive.material;

			// POSITION
			if (primitive.attributes.count("POSITION"))
			{
				const auto& acc = gltfModel.accessors[primitive.attributes.at("POSITION")];
				const auto& bv = gltfModel.bufferViews[acc.bufferView];
				const auto& buf = gltfModel.buffers[bv.buffer];
				const float* src = reinterpret_cast<const float*>(
					buf.data.data() + bv.byteOffset + acc.byteOffset);

				mesh.vertices.resize(acc.count);
				for (size_t i = 0; i < acc.count; i++)
				{
					mesh.vertices[i].position = { src[i * 3 + 0], src[i * 3 + 1], src[i * 3 + 2] };
				}
			}

			// NORMAL
			if (primitive.attributes.count("NORMAL"))
			{
				const auto& acc = gltfModel.accessors[primitive.attributes.at("NORMAL")];
				const auto& bv = gltfModel.bufferViews[acc.bufferView];
				const auto& buf = gltfModel.buffers[bv.buffer];
				const float* src = reinterpret_cast<const float*>(
					buf.data.data() + bv.byteOffset + acc.byteOffset);

				for (size_t i = 0; i < acc.count; i++)
				{
					mesh.vertices[i].normal = { src[i * 3 + 0], src[i * 3 + 1], src[i * 3 + 2] };
				}
			}

			// TEXCOORD_0
			if (primitive.attributes.count("TEXCOORD_0"))
			{
				const auto& acc = gltfModel.accessors[primitive.attributes.at("TEXCOORD_0")];
				const auto& bv = gltfModel.bufferViews[acc.bufferView];
				const auto& buf = gltfModel.buffers[bv.buffer];
				const float* src = reinterpret_cast<const float*>(
					buf.data.data() + bv.byteOffset + acc.byteOffset);

				for (size_t i = 0; i < acc.count; i++)
					mesh.vertices[i].texcoord =
				{ src[i * 2 + 0], src[i * 2 + 1] };
			}

			// WEIGHTS_0
			if (primitive.attributes.count("WEIGHTS_0"))
			{
				const auto& acc = gltfModel.accessors[
					primitive.attributes.at("WEIGHTS_0")];
				const auto& bv = gltfModel.bufferViews[acc.bufferView];
				const auto& buf = gltfModel.buffers[bv.buffer];
				const float* src = reinterpret_cast<const float*>(
					buf.data.data() + bv.byteOffset + acc.byteOffset);

				for (size_t i = 0; i < acc.count; i++)
				{
					mesh.vertices[i].boneWeight = { src[i * 4 + 0], src[i * 4 + 1], src[i * 4 + 2], src[i * 4 + 3] };
				}
			}

			// JOINTS_0
			if (primitive.attributes.count("JOINTS_0"))
			{
				const auto& acc = gltfModel.accessors[primitive.attributes.at("JOINTS_0")];
				const auto& bv = gltfModel.bufferViews[acc.bufferView];
				const auto& buf = gltfModel.buffers[bv.buffer];
				const uint16_t* src = reinterpret_cast<const uint16_t*>(
					buf.data.data() + bv.byteOffset + acc.byteOffset);

				//for (size_t i = 0; i < acc.count; i++)
				//{
				//	mesh.vertices[i].boneIndex = { src[i * 4 + 0], src[i * 4 + 1], src[i * 4 + 2], src[i * 4 + 3] };
				//}

				if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
				{
					// uint8_t 型（1バイト×4）
					for (size_t i = 0; i < acc.count; i++)
					{
						mesh.vertices[i].boneIndex = {
							src[i * 4 + 0],
							src[i * 4 + 1],
							src[i * 4 + 2],
							src[i * 4 + 3]
						};
					}
				}
				else // UNSIGNED_SHORT（uint16_t）
				{
					const uint16_t* src16 = reinterpret_cast<const uint16_t*>(src);
					for (size_t i = 0; i < acc.count; i++)
					{
						mesh.vertices[i].boneIndex = {
							src16[i * 4 + 0],
							src16[i * 4 + 1],
							src16[i * 4 + 2],
							src16[i * 4 + 3]
						};
					}
				}
			}

			// インデックス
			if (primitive.indices >= 0)
			{
				const auto& acc = gltfModel.accessors[primitive.indices];
				const auto& bv = gltfModel.bufferViews[acc.bufferView];
				const auto& buf = gltfModel.buffers[bv.buffer];
				const void* src = buf.data.data() +
					bv.byteOffset + acc.byteOffset;

				mesh.indices.resize(acc.count);

				if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
				{
					const uint16_t* s = static_cast<const uint16_t*>(src);
					for (size_t i = 0; i < acc.count; i++)
					{
						mesh.indices[i] = s[i];
					}
				}
				else if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
				{
					const uint32_t* s = static_cast<const uint32_t*>(src);
					for (size_t i = 0; i < acc.count; i++)
					{
						mesh.indices[i] = s[i];
					}
				}
			}

			// ボーン（スキンから取得）
			if (!gltfModel.skins.empty())
			{
				const auto& skin = gltfModel.skins[0];
				mesh.bones.resize(skin.joints.size());

				for (int j = 0; j < (int)skin.joints.size(); j++)
				{
					mesh.bones[j].nodeIndex = skin.joints[j];
				}

				// inverseBindMatrices
				if (skin.inverseBindMatrices >= 0)
				{
					const auto& acc = gltfModel.accessors[skin.inverseBindMatrices];
					const auto& bv = gltfModel.bufferViews[acc.bufferView];
					const auto& buf = gltfModel.buffers[bv.buffer];
					const float* src = reinterpret_cast<const float*>(
						buf.data.data() + bv.byteOffset + acc.byteOffset);

					for (int j = 0; j < (int)skin.joints.size(); j++)
					{
						memcpy(&mesh.bones[j].offsetTransform,
							src + j * 16,
							sizeof(DirectX::XMFLOAT4X4));
					}
				}
			}

			// タンジェント計算
			ComputeTangents(mesh.vertices, mesh.indices);

			data.meshes.push_back(std::move(mesh));
		}
	}
}

// マテリアル読み込み
void GLTFImporter::LoadMaterials(
	GLTFData& data,
	const tinygltf::Model& gltfModel, const std::filesystem::path& filepath)
{
	data.materials.reserve(gltfModel.materials.size());

	for (const auto& gltfMaterial : gltfModel.materials)
	{
		GLTFData::MaterialData mat;
		mat.name = gltfMaterial.name;

		const auto& pbr = gltfMaterial.pbrMetallicRoughness;

		if (!pbr.baseColorFactor.empty())
		{
			mat.baseColor =
			{
				static_cast<float>(pbr.baseColorFactor[0]),
				static_cast<float>(pbr.baseColorFactor[1]),
				static_cast<float>(pbr.baseColorFactor[2]),
				static_cast<float>(pbr.baseColorFactor[3])
			};
		}

		mat.roughnessFactor = static_cast<float>(pbr.roughnessFactor);
		mat.metallicFactor = static_cast<float>(pbr.metallicFactor);

		if (gltfMaterial.alphaMode == "BLEND")
		{
			mat.alphaMode = GLTFData::MaterialData::AlphaMode::Blend;
		}
		else if (gltfMaterial.alphaMode == "MASK")
		{
			mat.alphaMode = GLTFData::MaterialData::AlphaMode::Mask;
		}

		mat.alphaCutoff = static_cast<float>(gltfMaterial.alphaCutoff);

		// テクスチャインデックス設定
		mat.baseMapIndex =
			pbr.baseColorTexture.index;
		mat.normalMapIndex =
			gltfMaterial.normalTexture.index;
		mat.metallicRoughnessMapIndex =
			pbr.metallicRoughnessTexture.index;
		mat.occlusionMapIndex =
			gltfMaterial.occlusionTexture.index;
		mat.emissiveMapIndex =
			gltfMaterial.emissiveTexture.index;

		data.materials.push_back(std::move(mat));
	}
}

// テクスチャ読み込み
void GLTFImporter::LoadTextures(
	GLTFData& data,
	const tinygltf::Model& gltfModel,
	const std::filesystem::path& filepath)
{
	//data.textures.reserve(gltfModel.textures.size());

	//for (const auto& gltfTexture : gltfModel.textures)
	//{
	//	GLTFData::TextureData texData;
	//	const auto& image = gltfModel.images[gltfTexture.source];

	//	if (!image.uri.empty())
	//	{
	//		// 外部ファイルの場合はファイルから読み込んでバイナリ化
	//		std::string texPath = (filepath.parent_path() / image.uri).string();

	//		std::ifstream ifs(texPath, std::ios::binary);
	//		_ASSERT_EXPR_A(ifs, "Texture file not found");

	//		texData.pixels = std::vector<uint8_t>(
	//			std::istreambuf_iterator<char>(ifs),
	//			std::istreambuf_iterator<char>()
	//		);
	//	}
	//	else if (image.bufferView >= 0)
	//	{
	//		// GLB埋め込みの場合はbufferViewからバイナリを取り出す
	//		const auto& bv = gltfModel.bufferViews[image.bufferView];
	//		const auto& buffer = gltfModel.buffers[bv.buffer];
	//		const uint8_t* begin = buffer.data.data() + bv.byteOffset;

	//		texData.pixels.assign(begin, begin + bv.byteLength);
	//	}

	//	data.textures.push_back(std::move(texData));
	//}

	LARGE_INTEGER freq, t0, t1;
	QueryPerformanceFrequency(&freq);
	char buf[256];

	data.textures.reserve(gltfModel.textures.size());

	for (int i = 0; i < (int)gltfModel.textures.size(); i++)
	{
		QueryPerformanceCounter(&t0);

		const auto& gltfTexture = gltfModel.textures[i];
		GLTFData::TextureData texData;
		const auto& image = gltfModel.images[gltfTexture.source];

		if (!image.uri.empty())
		{
			//std::string texPath = (filepath.parent_path() / image.uri).string();
			//std::ifstream ifs(texPath, std::ios::binary);
			//_ASSERT_EXPR_A(ifs, "Texture file not found");
			//texData.pixels = std::vector<uint8_t>(
			//	std::istreambuf_iterator<char>(ifs),
			//	std::istreambuf_iterator<char>()
			//);

			std::string texPath = (filepath.parent_path() / image.uri).string();

			// istreambuf_iterator より read の方が大幅に速い
			std::ifstream ifs(texPath, std::ios::binary | std::ios::ate);
			_ASSERT_EXPR_A(ifs, "Texture file not found");

			std::streamsize size = ifs.tellg();
			ifs.seekg(0, std::ios::beg);

			texData.pixels.resize(static_cast<size_t>(size));
			ifs.read(reinterpret_cast<char*>(texData.pixels.data()), size);
		}
		else if (image.bufferView >= 0)
		{
			const auto& bv = gltfModel.bufferViews[image.bufferView];
			const auto& buffer = gltfModel.buffers[bv.buffer];
			const uint8_t* begin = buffer.data.data() + bv.byteOffset;
			texData.pixels.assign(begin, begin + bv.byteLength);
		}

		data.textures.push_back(std::move(texData));

		QueryPerformanceCounter(&t1);
		double elapsed = (double)(t1.QuadPart - t0.QuadPart) / freq.QuadPart * 1000.0;
		size_t sizeKB = data.textures.back().pixels.size() / 1024;
		sprintf_s(buf, "[GLTF]   texture[%d] uri=%s size=%zuKB : %.1f ms\n",
			i,
			gltfModel.images[gltfTexture.source].uri.c_str(),
			sizeKB,
			elapsed);
		OutputDebugStringA(buf);
	}
}

// アニメーション読み込み
void GLTFImporter::LoadAnimations(
	GLTFData& data,
	const tinygltf::Model& gltfModel)
{
	for (const auto& gltfAnim : gltfModel.animations)
	{
		GLTFData::Animation anim;
		anim.name = gltfAnim.name;
		anim.secondsLength = 0.0f;
		anim.nodeAnims.resize(data.nodes.size());

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
				inputBuf.data.data() + inputBV.byteOffset + inputAcc.byteOffset);
			const float* values = reinterpret_cast<const float*>(
				outputBuf.data.data() + outputBV.byteOffset + outputAcc.byteOffset);

			int nodeIndex = channel.target_node;
			if (nodeIndex < 0 || nodeIndex >= (int)anim.nodeAnims.size())
			{
				continue;
			}

			auto& nodeAnim = anim.nodeAnims[nodeIndex];

			if (inputAcc.count > 0)
			{
				anim.secondsLength = (std::max)(
					anim.secondsLength, times[inputAcc.count - 1]);
			}

			if (channel.target_path == "translation")
			{
				for (size_t i = 0; i < inputAcc.count; i++)
				{
					GLTFData::VectorKeyframe kf;
					kf.seconds = times[i];
					kf.value = { values[i * 3 + 0], values[i * 3 + 1], values[i * 3 + 2] };
					nodeAnim.positionKeyframes.push_back(kf);
				}
			}
			else if (channel.target_path == "rotation")
			{
				for (size_t i = 0; i < inputAcc.count; i++)
				{
					// なぜかUnityで出力したアニメーションデータにはゴミと思われるキーフレームが存在している場合がある。
					// 小数点が存在するフレーム（時間）がゴミデータっぽいので除外する。
					float frame = times[i] * 60.0f;
					if (fabs(std::round(frame) - frame) > 0.001) continue;

					GLTFData::QuaternionKeyframe kf;
					kf.seconds = times[i];
					kf.value = { values[i * 4 + 0], values[i * 4 + 1],
								   values[i * 4 + 2], values[i * 4 + 3] };
					nodeAnim.rotationKeyframes.push_back(kf);
				}
			}
			else if (channel.target_path == "scale")
			{
				for (size_t i = 0; i < inputAcc.count; i++)
				{
					GLTFData::VectorKeyframe kf;
					kf.seconds = times[i];
					kf.value = { values[i * 3 + 0], values[i * 3 + 1], values[i * 3 + 2] };
					nodeAnim.scaleKeyframes.push_back(kf);
				}
			}
		}
		data.animations.push_back(std::move(anim));
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
	std::vector<GLTFData::Vertex>& vertices,
	const std::vector<uint32_t>& indices)
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

		DirectX::XMVECTOR e1 = DirectX::XMVectorSubtract(p1, p0);
		DirectX::XMVECTOR e2 = DirectX::XMVectorSubtract(p2, p0);
		DirectX::XMVECTOR du1 = DirectX::XMVectorSubtract(uv1, uv0);
		DirectX::XMVECTOR du2 = DirectX::XMVectorSubtract(uv2, uv0);

		float u1 = DirectX::XMVectorGetX(du1);
		float v1f = DirectX::XMVectorGetY(du1);
		float u2 = DirectX::XMVectorGetX(du2);
		float v2f = DirectX::XMVectorGetY(du2);
		float denom = u1 * v2f - u2 * v1f;

		if (fabsf(denom) < 1e-6f) continue;

		float r = 1.0f / denom;
		DirectX::XMVECTOR tangent = DirectX::XMVector3Normalize(
			DirectX::XMVectorScale(
				DirectX::XMVectorSubtract(
					DirectX::XMVectorScale(e1, v2f),
					DirectX::XMVectorScale(e2, v1f)),
				r));

		DirectX::XMFLOAT3 t;
		DirectX::XMStoreFloat3(&t, tangent);
		v0.tangent = v1.tangent = v2.tangent = { t.x, t.y, t.z, 1.0f };
	}
}


// グローバルトランスフォーム計算
void GLTFImporter::CalcGlobalTransform(
	std::vector<GLTFData::Node>& nodes,
	int nodeIndex,
	const DirectX::XMFLOAT4X4& parentTransform)
{
	auto& node = nodes[nodeIndex];

	DirectX::XMMATRIX S = DirectX::XMMatrixScaling(
		node.scale.x, node.scale.y, node.scale.z);
	DirectX::XMMATRIX R = DirectX::XMMatrixRotationQuaternion(
		DirectX::XMVectorSet(
			node.rotation.x, node.rotation.y,
			node.rotation.z, node.rotation.w));
	DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(
		node.position.x, node.position.y, node.position.z);

	DirectX::XMMATRIX local = S * R * T;
	DirectX::XMMATRIX parent = DirectX::XMLoadFloat4x4(&parentTransform);
	DirectX::XMMATRIX global = local * parent;

	DirectX::XMStoreFloat4x4(&node.localTransform, local);
	DirectX::XMStoreFloat4x4(&node.globalTransform, global);

	for (int child : node.childIndices)
	{
		CalcGlobalTransform(nodes, child, node.globalTransform);
	}
}
