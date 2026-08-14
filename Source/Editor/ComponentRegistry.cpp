#include "ComponentRegistry.h"

std::unordered_map<std::string, ComponentInfo> ComponentRegistry::registry;
std::vector<std::string> ComponentRegistry::names;
std::unordered_map<std::string, std::string> ComponentRegistry::typeNameToDisplayName;

void ComponentRegistry::Register(const std::string& name, ComponentInfo info)
{
    if (registry.find(name) != registry.end()) return;
    info.name = name;
    registry[name] = info;
    names.push_back(name);
}

const std::vector<std::string>& ComponentRegistry::GetNames()
{
    return names;
}

const ComponentInfo* ComponentRegistry::Get(const std::string& name)
{
    auto it = registry.find(name);
    if (it == registry.end()) return nullptr;
    return &it->second;
}

void ComponentRegistry::RegisterTypeName(const std::string& typeName, const std::string& displayName)
{
    typeNameToDisplayName[typeName] = displayName;
}

const ComponentInfo* ComponentRegistry::GetByTypeName(const std::string& typeName)
{
    auto it = typeNameToDisplayName.find(typeName);
    if (it == typeNameToDisplayName.end()) return nullptr;
    return Get(it->second);
}