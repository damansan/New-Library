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
    std::function<void(entt::registry&, entt::entity)> drawImGui;
};

class ComponentRegistry
{
public:
    static void Register(const std::string& name, ComponentInfo info);
    static const std::vector<std::string>& GetNames();
    static const ComponentInfo* Get(const std::string& name);

private:
    static std::unordered_map<std::string, ComponentInfo> registry;
    static std::vector<std::string> names;
};

#define REGISTER_COMPONENT(Type, Name, DrawFunc)                                    \
    ComponentRegistry::Register(Name,                                               \
    {                                                                               \
        Name,                                                                       \
        [](auto& r, auto e){ if (!r.any_of<Type>(e)) r.emplace<Type>(e); },         \
        [](auto& r, auto e){ r.remove<Type>(e); },                                  \
        [](auto& r, auto e){ return r.any_of<Type>(e); },                           \
        DrawFunc                                                                    \
    })