#include "EntitySerializer.h"
#include "Engine/Resource/ComponentRegistry.h"
#include "Engine/ECS/Components/PrefabInstanceComponent.h"
#include "Engine/Core/Log.h"

#include "Tool/Editor/MetaRegistry.h"

#include <DirectXMath.h>
#include <fstream>

nlohmann::json EntitySerializer::SerializeEntity(entt::registry& registry, entt::entity e)
{
	nlohmann::json result = nlohmann::json::object();

	for (const auto& name : ComponentRegistry::GetNames())
	{
		const ComponentInfo* info = ComponentRegistry::Get(name);
		if (!info || !info->has(registry, e)) continue;
		if (!info->getMeta) continue;

		// instanceをローカル変数に代入すると参照が崩れるため、type()の取得と
		// フィールドの読み取りで、それぞれその場でgetMeta()を呼び直す
		entt::meta_type metaType = info->getMeta(registry, e).type();
		if (!metaType) continue;

		nlohmann::json fields = nlohmann::json::object();

		for (auto&& [id, metaData] : metaType.data())
		{
			// readOnlyの項目は、実行中にSystemが書き換える状態なので、保存しない
			FieldInfo* fieldInfo = metaData.custom();
			if (fieldInfo && fieldInfo->readOnly) continue;

			std::string fieldName(metaData.name());
			entt::meta_any value = metaData.get(info->getMeta(registry, e));

			// 保存に未対応の型は、書き出さない
			nlohmann::json serialized = SerializeMetaAny(value);
			if (serialized.is_null()) continue;

			fields[fieldName] = serialized;
		}

		result[name] = fields;
	}

	return result;
}

nlohmann::json EntitySerializer::SerializeMetaAny(entt::meta_any& value)
{
	if (float* f = value.try_cast<float>()) return *f;
	if (int* i = value.try_cast<int>()) return *i;
	if (bool* b = value.try_cast<bool>()) return *b;
	if (std::string* s = value.try_cast<std::string>()) return *s;

	if (DirectX::XMFLOAT2* v = value.try_cast<DirectX::XMFLOAT2>())
		return nlohmann::json::array({ v->x, v->y });
	if (DirectX::XMFLOAT3* v = value.try_cast<DirectX::XMFLOAT3>())
		return nlohmann::json::array({ v->x, v->y, v->z });
	if (DirectX::XMFLOAT4* v = value.try_cast<DirectX::XMFLOAT4>())
		return nlohmann::json::array({ v->x, v->y, v->z, v->w });

	if (value.type().is_enum())
	{
		// enumの値を、REFLECT_ENUM_VALUEで登録した名前(文字列)として保存する
		entt::meta_type enumType = value.type();
		for (auto&& [id, enumData] : enumType.data())
		{
			entt::meta_any constValue = enumData.get({});
			if (constValue == value)
			{
				return std::string(enumData.name());
			}
		}
	}

	return nullptr; // 対応していない型
}

void EntitySerializer::DeserializeEntity(entt::registry& registry, entt::entity e, const nlohmann::json& data)
{
	for (auto& [name, fields] : data.items())
	{
		const ComponentInfo* info = ComponentRegistry::Get(name);
		if (!info)
		{
			// 削除・改名されたコンポーネント。落とさずに飛ばして、ログに残す
			Log(u8"未登録のコンポーネントを無視しました: %s", name.c_str());
			continue;
		}

		info->add(registry, e); // 無ければ付与
		if (!info->getMeta) continue;

		entt::meta_any instance = info->getMeta(registry, e);
		if (!instance) continue;

		entt::meta_type metaType = instance.type();

		for (auto&& [id, metaData] : metaType.data())
		{
			// readOnlyの項目は、ファイルに入っていても書き戻さない
			FieldInfo* fieldInfo = metaData.custom();
			if (fieldInfo && fieldInfo->readOnly) continue;

			std::string fieldName(metaData.name());
			if (!fields.contains(fieldName)) continue; // JSONに無いフィールドは初期値のまま
			DeserializeMetaAny(metaData, instance, fields[fieldName]);
		}
	}
}

// JSONファイルを読み込む。ファイルが無い・壊れている場合は、falseを返す
bool EntitySerializer::LoadJsonFile(const std::string& path, nlohmann::json& out)
{
	std::ifstream ifs(path);
	if (!ifs)
	{
		Log(u8"ファイルを開けません: %s", path.c_str());
		return false;
	}

	// 例外を無効にして、壊れたファイルでもアプリを落とさない
	out = nlohmann::json::parse(ifs, nullptr, false);
	if (out.is_discarded())
	{
		Log(u8"JSONの形式が不正です: %s", path.c_str());
		return false;
	}
	return true;
}

// プレハブファイルからEntityを生成する。成功したらそのEntityを、失敗したらentt::nullを返す
entt::entity EntitySerializer::InstantiatePrefab(entt::registry& registry, const std::string& prefabPath)
{
	nlohmann::json data;
	if (!LoadJsonFile(prefabPath, data)) return entt::null;

	if (!data.is_object())
	{
		Log(u8"プレハブの形式が不正です: %s", prefabPath.c_str());
		return entt::null;
	}

	entt::entity e = registry.create();

	try
	{
		DeserializeEntity(registry, e, data);
	}
	catch (const nlohmann::json::exception& ex)
	{
		// 値の型が想定と違う場合など。作りかけのEntityは残さない
		Log(u8"プレハブの復元中にエラー: %s (%s)", prefabPath.c_str(), ex.what());
		registry.destroy(e);
		return entt::null;
	}

	// どのプレハブから生まれたかを記録する
	registry.emplace<PrefabInstanceComponent>(e, prefabPath);
	return e;
}

void EntitySerializer::DeserializeMetaAny(entt::meta_data metaData, entt::meta_any& instance, const nlohmann::json& value)
{
	entt::meta_any current = metaData.get(instance);

	if (current.try_cast<float>() && value.is_number())
	{
		metaData.set(instance, value.get<float>());
	}
	else if (current.try_cast<int>() && value.is_number_integer())
	{
		metaData.set(instance, value.get<int>());
	}
	else if (current.try_cast<bool>() && value.is_boolean())
	{
		metaData.set(instance, value.get<bool>());
	}
	else if (current.try_cast<std::string>() && value.is_string())
	{
		metaData.set(instance, value.get<std::string>());
	}
	else if (current.try_cast<DirectX::XMFLOAT2>() && value.is_array() && value.size() == 2)
	{
		metaData.set(instance, DirectX::XMFLOAT2{ value[0].get<float>(), value[1].get<float>() });
	}
	else if (current.try_cast<DirectX::XMFLOAT3>() && value.is_array() && value.size() == 3)
	{
		metaData.set(instance, DirectX::XMFLOAT3{ value[0].get<float>(), value[1].get<float>(), value[2].get<float>() });
	}
	else if (current.try_cast<DirectX::XMFLOAT4>() && value.is_array() && value.size() == 4)
	{
		metaData.set(instance, DirectX::XMFLOAT4{ value[0].get<float>(), value[1].get<float>(), value[2].get<float>(), value[3].get<float>() });
	}
	else if (current.type().is_enum() && value.is_string())
	{
		// 保存されていた名前から、対応するenum値を探して書き戻す
		entt::meta_type enumType = current.type();
		std::string targetName = value.get<std::string>();

		for (auto&& [id, enumData] : enumType.data())
		{
			if (std::string(enumData.name()) == targetName)
			{
				metaData.set(instance, enumData.get({}));
				break;
			}
		}
	}
}