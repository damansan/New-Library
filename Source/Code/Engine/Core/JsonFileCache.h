#pragma once
#include "Engine/Core/Log.h"

#include <string>
#include <unordered_map>
#include <functional>
#include <fstream>
#include <filesystem>
#include <json.hpp>

template<typename T>
class JsonFileCache
{
public:
	T& GetOrLoad(const std::string& path, const std::function<void(T&, const nlohmann::json&)>& loadFunc)
	{
		std::error_code ec;
		auto writeTime = std::filesystem::last_write_time(path, ec);
		const bool exists = !ec;

		auto it = cache.find(path);
		if (it != cache.end())
		{
			const Entry& cached = it->second;

			// 前回と同じ状態なら、キャッシュをそのまま返す
			if (exists == cached.exists && (!exists || writeTime == cached.lastWriteTime))
			{
				return it->second.data;
			}
			// ファイルが更新された・現れた・消えた場合は、読み込みをやり直す
			cache.erase(it);
		}

		// マップの中に直接構築してから読み込む
		auto result = cache.try_emplace(path);
		Entry& entry = result.first->second;
		entry.lastWriteTime = writeTime;
		entry.exists = exists;

		// 状態が変わったときだけここに来るので、ログは1回だけ出る
		if (!exists)
		{
			Log(u8" ファイルが見つかりません: %s", path.c_str());
			return entry.data;
		}

		std::ifstream ifs(path);
		if (!ifs.is_open())
		{
			Log(u8" ファイルを開けません: %s", path.c_str());
			return entry.data;
		}

		try
		{
			nlohmann::json j;
			ifs >> j;
			loadFunc(entry.data, j);
		}
		catch (const std::exception& e)
		{
			Log(u8" JSONの読み込みに失敗: %s (%s)", path.c_str(), e.what());
		}

		return entry.data;
	}

private:
	struct Entry
	{
		T data;
		std::filesystem::file_time_type lastWriteTime;
		bool exists = false; // 読み込んだときにファイルがあったか
	};

	std::unordered_map<std::string, Entry> cache;
};