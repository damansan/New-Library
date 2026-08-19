#include "ResourceManager.h"
#include "GLTFImporter.h"
#include "GLTFConverter.h" 
#include "ModelSerializer.h" 
#include "GpuResourceUtils.h" //（GPU再生成用）

// モデル取得
std::shared_ptr<Model> ResourceManager::GetModel(ID3D11Device* device, const std::string& filename)
{
	// キャッシュ検索
	auto it = modelCache.find(filename);
	if (it != modelCache.end())
	{
		// キャッシュが有効なら使い回す
		return it->second;
	}

	//// GLTFImporter でファイルを読み込み GLTFData を生成
	//auto gltfData = GLTFImporter::Load(filename);

	//// GLTFConverter で GLTFData を Model に変換
	//auto model = GLTFConverter::Convert(device, gltfData);

	std::shared_ptr<Model> model;

	// .cereal がある(.cerealを読み込む)
	if (ModelSerializer::Exists(filename))
	{
		model = std::make_shared<Model>();
		ModelSerializer::Load(*model, ModelSerializer::ToCerealPath(filename));
		GLTFConverter::CreateGpuResources(device, *model);
	}
	// .cereal がない(GLTFを読み込む)
	else
	{
		auto gltfData = GLTFImporter::Load(filename);
		model = GLTFConverter::Convert(device, gltfData);
		ModelSerializer::Save(*model, ModelSerializer::ToCerealPath(filename));
	}

	// キャッシュに登録
	modelCache[filename] = model;

	return model;
}

// キャッシュ削除
void ResourceManager::ReleaseModel(const std::string& filename)
{
	modelCache.erase(filename);
}

// 全キャッシュ削除
void ResourceManager::ReleaseAll()
{
	modelCache.clear();
}
