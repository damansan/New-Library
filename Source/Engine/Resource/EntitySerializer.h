#pragma once
#include "entt.hpp"
#include <json.hpp>

class EntitySerializer
{
public:
	// Entity1つを、コンポーネント名をキーにしたJSONへ変換する
	static nlohmann::json SerializeEntity(entt::registry& registry, entt::entity e);

	// JSONの内容を、Entityへ反映する(コンポーネントを付与し、フィールドを書き戻す)
	static void DeserializeEntity(entt::registry& registry, entt::entity e, const nlohmann::json& data);

private:
	// entt::meta_any 1つ分のフィールド値を、JSONへ変換する
	static nlohmann::json SerializeMetaAny(entt::meta_any& value);
	// JSONの値を、entt::meta経由でフィールドへ書き戻す
	static void DeserializeMetaAny(entt::meta_data metaData, entt::meta_any& instance, const nlohmann::json& value);
};