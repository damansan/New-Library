#pragma once
#include <windows.h>
#include <Xinput.h>
#pragma comment(lib, "Xinput.lib")
#include "entt.hpp"

class InputSystem
{
public:
	void Update(entt::registry& registry);

private:
	// デッドゾーン処理
	float ApplyDeadzone(float value, float deadzone = 0.2f);
};