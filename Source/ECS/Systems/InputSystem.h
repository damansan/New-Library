#pragma once
#pragma comment(lib, "Xinput.lib")
#include <windows.h>
#include <Xinput.h>
#include <DualSenseWindows/IO.h>
#include "entt.hpp"

#include "ECS/Components/InputComponent.h"

class InputSystem
{
public:
	InputSystem();
	~InputSystem();

	void Update(entt::registry& registry,float elapsedTime);

private:
	// デッドゾーン処理
	float ApplyDeadzone(float value, float deadzone = 0.2f);
	
	// Xbox 入力
	void UpdateXbox(InputComponent& input);

	// PS5 入力
	void UpdatePS5(InputComponent& input);

	// Xbox
	bool xboxConnected = false;
	XINPUT_STATE xState = {};

	// PS5
	DS5W::DeviceContext ds5Context;
	bool ds5Connected = false;
	DS5W::DS5InputState ds5State = {};
	// PS5 再検索タイマー
	float ds5ReconnectTimer = 0.0f;
	const float ds5ReconnectInterval = 3.0f; // 3秒に1回
};