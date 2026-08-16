#pragma once
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

		auto it = cache.find(path);
		if (it != cache.end())
		{
			// ファイル自体が消えている場合は、今のキャッシュをそのまま返す
			if (!ec && writeTime == it->second.lastWriteTime)
			{
				return it->second.data;
			}
			// 更新されていた場合は、既存のエントリを一旦消して、下の新規読み込みへ進む
			cache.erase(it);
		}

		// マップの中に直接構築してから読み込む
		auto result = cache.try_emplace(path);
		Entry& entry = result.first->second;
		entry.lastWriteTime = writeTime;

		std::ifstream ifs(path);
		if (ifs.is_open())
		{
			try
			{
				nlohmann::json j;
				ifs >> j;
				loadFunc(entry.data, j);
			}
			catch (const std::exception&) {}
		}

		return entry.data;
	}

private:
	struct Entry
	{
		T data;
		std::filesystem::file_time_type lastWriteTime;
	};

	std::unordered_map<std::string, Entry> cache;
};