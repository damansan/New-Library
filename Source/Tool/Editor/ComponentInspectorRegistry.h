#pragma once
#include <string>
#include <unordered_map>
#include <functional>
#include "entt.hpp"

class ComponentInspectorRegistry
{
public:
	static void Register(const std::string& name, std::function<void(entt::registry&, entt::entity)> drawImGui);
	static const std::function<void(entt::registry&, entt::entity)>* Get(const std::string& name);

private:
	static std::unordered_map<std::string, std::function<void(entt::registry&, entt::entity)>> registry;
};