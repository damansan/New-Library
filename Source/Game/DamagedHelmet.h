#pragma once
#include "Game/Object.h"

class DamagedHelmet : public Object
{
public:
	DamagedHelmet() = default;
	virtual ~DamagedHelmet() = default;

	void Initialize(entt::registry& registry, const SceneContext& context) override;
	void RenderImGui() override;
};