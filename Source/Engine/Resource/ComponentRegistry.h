#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include "entt.hpp"

struct ComponentInfo
{
    std::string name;
    std::function<void(entt::registry&, entt::entity)> add;
    std::function<void(entt::registry&, entt::entity)> remove;
    std::function<bool(entt::registry&, entt::entity)> has;

	// エンティティが持つコンポーネントの実態をmeta_anyとして取得する
    std::function<entt::meta_any(entt::registry&, entt::entity)> getMeta;
};

class ComponentRegistry
{
public:
    static void Register(const std::string& name, ComponentInfo info);
    static const std::vector<std::string>& GetNames();
    static const ComponentInfo* Get(const std::string& name);

    static void RegisterTypeName(const std::string& typeName, const std::string& displayName);
    static const ComponentInfo* GetByTypeName(const std::string& typeName);

private:
    static std::unordered_map<std::string, ComponentInfo> registry;
    static std::vector<std::string> names;

    static std::unordered_map<std::string, std::string> typeNameToDisplayName;
};