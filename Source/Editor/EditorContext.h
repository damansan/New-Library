#pragma once
#include <string>
#include <unordered_map>
#include "entt.hpp"

struct EditorContext
{
    entt::entity selectedEntity = entt::null;
    std::unordered_map<entt::entity, std::string> entityNames;

    std::string GetName(entt::entity e) const
    {
        auto it = entityNames.find(e);
        if (it != entityNames.end()) return it->second;
        return "Entity " + std::to_string((uint32_t)e);
    }

    void SetName(entt::entity e, const std::string& name)
    {
        entityNames[e] = name;
    }

    void Select(entt::entity e) { selectedEntity = e; }
    void Deselect() { selectedEntity = entt::null; }

    bool IsSelected(entt::entity e) const
    {
        return selectedEntity == e;
    }
};