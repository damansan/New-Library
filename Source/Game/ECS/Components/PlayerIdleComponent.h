#pragma once
#include "Engine/ECS/Reflect.h"

enum class PlayerIdleSubState
{
	None,
	Idle,   // ‘Ò‹@’†
};

struct PlayerIdleComponent
{
	// ó‘Ô
	PlayerIdleSubState idleSub = PlayerIdleSubState::None;

	REFLECT_BEGIN(PlayerIdleComponent, "Player Idle")

		// ó‘Ôƒtƒ‰ƒO
		REFLECT_FIELD(bool, isIdling, (false), ("Is Idling", 0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient))

	REFLECT_END()
};