#include "ResourceManager.h"
#include "GLTFImporter.h"

// モデル取得
std::shared_ptr<Model> ResourceManager::GetModel(ID3D11Device* device, const std::string& filename)
{
	// キャッシュ検索
	auto it = modelCache.find(filename);
	if (it != modelCache.end())
	{
		// キャッシュが有効なら使い回す
		auto cached = it->second.lock();
		if (cached) return cached;
	}

	// 未ロードまたはキャッシュ切れの場合は新規読み込み
	auto model = GLTFImporter::Load(device, filename);

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
