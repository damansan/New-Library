#include "ComponentRegistry.h"

std::unordered_map<std::string, ComponentInfo> ComponentRegistry::registry;
std::vector<std::string> ComponentRegistry::names;

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