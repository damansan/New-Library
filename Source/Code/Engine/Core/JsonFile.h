#pragma once
#include <string>
#include <fstream>
#include <filesystem>
#include <json.hpp>
#include "Engine/Core/Log.h"

namespace JsonFile
{
	// data.FromJson に渡す
	template<typename T>
	bool Load(const std::string& path, T& data)
	{
		std::ifstream ifs(path);
		if (!ifs.is_open()) return false;

		try
		{
			nlohmann::json j;
			ifs >> j;
			data.FromJson(j);
			return true;
		}
		catch (const std::exception& e)
		{
			Log(u8" JSONの読み込みに失敗: %s (%s)", path.c_str(), e.what());
			return false;
		}
	}

	// data.ToJson を書き出す
	template<typename T>
	bool Save(const std::string& path, const T& data)
	{
		std::error_code ec;
		std::filesystem::create_directories(std::filesystem::path(path).parent_path(), ec);

		std::ofstream ofs(path);
		if (!ofs.is_open())
		{
			Log(u8" ファイルを保存できません: %s", path.c_str());
			return false;
		}

		ofs << data.ToJson().dump(4);
		return true;
	}
}