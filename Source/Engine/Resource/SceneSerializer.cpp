#include "SceneSerializer.h"
#include "Engine/Resource/EntitySerializer.h"
#include "Engine/ECS/Components/PrefabInstanceComponent.h"
#include "Engine/Core/Log.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <vector>

nlohmann::json SceneSerializer::Serialize(entt::registry& registry, const NameGetter& getName)
{
	nlohmann::json scene = nlohmann::json::object();
	scene["version"] = Version;
	scene["settings"] = nlohmann::json::object(); // 環境設定（ライト・SkyBoxなど）の置き場所。今は空

	std::vector<entt::entity> entities;
	for (auto e : registry.view<entt::entity>())
	{
		entities.push_back(e);
	}
	std::reverse(entities.begin(), entities.end());

	nlohmann::json entries = nlohmann::json::array();
	for (auto e : entities)
	{
		nlohmann::json entry = nlohmann::json::object();

		if (getName)
		{
			entry["name"] = getName(e);
		}

		if (auto* prefab = registry.try_get<PrefabInstanceComponent>(e))
		{
			// プレハブ配置のEntityは、現在値とプレハブの値の差分だけを書く
			entry["prefab"] = prefab->prefabPath;

			nlohmann::json prefabData;
			if (EntitySerializer::LoadJsonFile(prefab->prefabPath, prefabData))
			{
				nlohmann::json current = EntitySerializer::SerializeEntity(registry, e);
				nlohmann::json overrides = DiffAgainstPrefab(current, prefabData);

				if (!overrides.empty())
				{
					entry["overrides"] = std::move(overrides);
				}
			}
			else
			{
				// プレハブ本体が読めない場合は、差分を取れないので、参照だけ残す
				Log(u8"[Warn] 差分比較のため、プレハブを読み込めませんでした: %s", prefab->prefabPath.c_str());
			}
		}
		else
		{
			// それ以外は、プレハブと同じ形式で、コンポーネントの中身を書く
			entry["components"] = EntitySerializer::SerializeEntity(registry, e);
		}

		entries.push_back(std::move(entry));
	}
	scene["entities"] = std::move(entries);

	return scene;
}

bool SceneSerializer::SaveToFile(entt::registry& registry, const std::string& path, const NameGetter& getName)
{
	nlohmann::json scene = Serialize(registry, getName);

	// 保存先のフォルダが無ければ作る
	std::filesystem::path filePath(path);
	if (filePath.has_parent_path())
	{
		std::error_code ec;
		std::filesystem::create_directories(filePath.parent_path(), ec);
	}

	std::ofstream ofs(filePath);
	if (!ofs)
	{
		Log(u8"[Error] シーンを書き込めません: %s", path.c_str());
		return false;
	}

	// UTF-8として不正な文字が名前・パスに入っていても、例外で落とさず置き換えて書き出す
	ofs << scene.dump(4, ' ', false, nlohmann::json::error_handler_t::replace);

	Log(u8"[Info] シーンを保存しました: %s (%d個のEntity)", path.c_str(), static_cast<int>(scene["entities"].size()));
	return true;
}

// JSONの内容で、Entityを作る
void SceneSerializer::Deserialize(entt::registry& registry, const nlohmann::json& scene, const NameSetter& setName)
{
	if (!scene.contains("entities") || !scene["entities"].is_array())
	{
		Log(u8"シーンの形式が不正です(entitiesがありません)");
		return;
	}

	int failedCount = 0;

	for (const auto& entry : scene["entities"])
	{
		entt::entity e = entt::null;

		if (entry.contains("prefab") && entry["prefab"].is_string())
		{
			e = EntitySerializer::InstantiatePrefab(registry, entry["prefab"].get<std::string>());
		}
		else if (entry.contains("components"))
		{
			e = EntitySerializer::CreateFromComponents(registry, entry["components"]);
		}

		if (e == entt::null)
		{
			// InstantiatePrefab / CreateFromComponents 側が、詳しい理由をLogに出している
			++failedCount;
			continue;
		}

		// プレハブ生成の後、差分があれば上書きする
		if (entry.contains("overrides") && entry["overrides"].is_object())
		{
			try
			{
				EntitySerializer::DeserializeEntity(registry, e, entry["overrides"]);
			}
			catch (const nlohmann::json::exception&)
			{
				Log(u8"[Warn] overridesの適用中にエラー: %s", entry.value("name", std::string()).c_str());
			}
		}

		if (setName && entry.contains("name") && entry["name"].is_string())
		{
			setName(e, entry["name"].get<std::string>());
		}
	}

	if (failedCount > 0)
	{
		Log(u8"%d個のEntityを復元できませんでした", failedCount);
	}
}

// ファイルから読み込む。読み込み前に、既存のEntityを1つずつdestroyする
bool SceneSerializer::LoadFromFile(entt::registry& registry, const std::string& path, const NameSetter& setName)
{
	nlohmann::json scene;
	if (!EntitySerializer::LoadJsonFile(path, scene)) return false;

	// 既存のEntityを、1つずつ破棄する
	std::vector<entt::entity> existing;
	for (auto e : registry.view<entt::entity>())
	{
		existing.push_back(e);
	}
	for (auto e : existing)
	{
		registry.destroy(e);
	}

	Deserialize(registry, scene, setName);

	Log(u8"シーンを読み込みました: %s", path.c_str());
	return true;
}

// 現在のEntityの値とプレハブでの値を比較する
nlohmann::json SceneSerializer::DiffAgainstPrefab(const nlohmann::json& current, const nlohmann::json& prefab)
{
	nlohmann::json overrides = nlohmann::json::object();

	// currentは、そのEntityが今持っているコンポーネントだけを含む
	for (auto& [componentName, currentFields] : current.items())
	{
		if (!currentFields.is_object()) continue;

		// プレハブ側に無いコンポーネント（配置後に追加した）は、比較のしようがないので対象外
		if (!prefab.contains(componentName)) continue;
		const nlohmann::json& prefabFields = prefab[componentName];
		if (!prefabFields.is_object()) continue;

		nlohmann::json fieldDiff = nlohmann::json::object();

		for (auto& [fieldName, value] : currentFields.items())
		{
			// プレハブ側に無いフィールド（改名・追加）は、比較できないので飛ばす
			if (!prefabFields.contains(fieldName)) continue;

			if (value != prefabFields[fieldName])
			{
				fieldDiff[fieldName] = value;
			}
		}

		if (!fieldDiff.empty())
		{
			overrides[componentName] = std::move(fieldDiff);
		}
	}

	return overrides;
}
