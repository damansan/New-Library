#include "ResourceManager.h"
#include "GLTFImporter.h"
#include "GLTFConverter.h" 

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

	// GLTFImporter でファイルを読み込み GLTFData を生成
	auto gltfData = GLTFImporter::Load(filename);

	// GLTFConverter で GLTFData を Model に変換
	auto model = GLTFConverter::Convert(device, gltfData);

	// キャッシュに登録
	modelCache[filename] = model;

	return model;

 //   // TODO デバッグ時間計測
	//auto it = modelCache.find(filename);
 //   if (it != modelCache.end())
 //   {
 //       auto cached = it->second.lock();
 //       if (cached) return cached;
 //   }

 //   LARGE_INTEGER freq, t0, t1;
 //   QueryPerformanceFrequency(&freq);
 //   char buf[256];

 //   // Import
 //   QueryPerformanceCounter(&t0);
 //   auto gltfData = GLTFImporter::Load(filename);
 //   QueryPerformanceCounter(&t1);
 //   sprintf_s(buf, "[GLTF] >>> Import total  : %.1f ms\n",
 //       (double)(t1.QuadPart - t0.QuadPart) / freq.QuadPart * 1000.0);
 //   OutputDebugStringA(buf);

 //   // Convert
 //   QueryPerformanceCounter(&t0);
 //   auto model = GLTFConverter::Convert(device, gltfData);
 //   QueryPerformanceCounter(&t1);
 //   sprintf_s(buf, "[GLTF] >>> Convert total : %.1f ms\n",
 //       (double)(t1.QuadPart - t0.QuadPart) / freq.QuadPart * 1000.0);
 //   OutputDebugStringA(buf);

 //   modelCache[filename] = model;
 //   return model;
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
