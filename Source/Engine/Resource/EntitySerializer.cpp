#include "EntitySerializer.h"
#include "Engine/Resource/ComponentRegistry.h"
#include <DirectXMath.h>

nlohmann::json EntitySerializer::SerializeEntity(entt::registry& registry, entt::entity e)
{
	nlohmann::json result = nlohmann::json::object();

	for (const auto& name : ComponentRegistry::GetNames())
	{
		const ComponentInfo* info = ComponentRegistry::Get(name);
		if (!info || !info->has(registry, e)) continue;
		if (!info->getMeta) continue; // entt::metaに未登録のコンポーネントはスキップ

		entt::meta_any instance = info->getMeta(registry, e);
		if (!instance) continue;

		entt::meta_type metaType = instance.type();
		nlohmann::json fields = nlohmann::json::object();

		for (auto&& [id, metaData] : metaType.data())
		{
			std::string fieldName(metaData.name());
			entt::meta_any value = metaData.get(instance);
			fields[fieldName] = SerializeMetaAny(value);
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

	return nullptr; // 対応していない型
}

void EntitySerializer::DeserializeEntity(entt::registry& registry, entt::entity e, const nlohmann::json& data)
{
	for (auto& [name, fields] : data.items())
	{
		const ComponentInfo* info = ComponentRegistry::Get(name);
		if (!info) continue; // 今は存在しないコンポーネント名は無視(削除されたコンポーネント対策)

		info->add(registry, e); // 無ければ付与
		if (!info->getMeta) continue;

		entt::meta_any instance = info->getMeta(registry, e);
		if (!instance) continue;

		entt::meta_type metaType = instance.type();

		for (auto&& [id, metaData] : metaType.data())
		{
			std::string fieldName(metaData.name());
			if (!fields.contains(fieldName)) continue; // JSONに無いフィールドは初期値のまま(無視)

			DeserializeMetaAny(metaData, instance, fields[fieldName]);
		}
	}
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
}