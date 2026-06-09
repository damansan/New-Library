#include "InputSystem.h"
#include "entt.hpp"
#include "ECS/Components/InputComponent.h"
#include <windows.h> 

void InputSystem::Update(entt::registry& registry)
{
    auto view = registry.view<InputComponent>();
    view.each([&](auto entity, InputComponent& input)
        {
            // --- 移動入力のリセット ---
            input.moveX = 0.0f;
            input.moveZ = 0.0f;

            // --- キーボード入力 ---
            if (GetKeyState('W') & 0x8000) input.moveZ += 1.0f;
            if (GetKeyState('S') & 0x8000) input.moveZ -= 1.0f;
            if (GetKeyState('A') & 0x8000) input.moveX -= 1.0f;
            if (GetKeyState('D') & 0x8000) input.moveX += 1.0f;

            // --- アクション入力 ---
            input.jump = (GetKeyState(VK_SPACE) & 0x8000) != 0;
            input.attack = (GetKeyState(VK_LBUTTON) & 0x8000) != 0;
        });
}
