#pragma once
#include "entt.hpp"
#include "Engine/ECS/Reflection.h"

class MetaRegistry
{
public:
	// “o˜^Ï‚İƒƒ^î•ñ‚©‚çInspectorUI‚ğ©“®•`‰æ
	static void DrawMetaFields(entt::meta_type metaType, entt::meta_any instance);
};