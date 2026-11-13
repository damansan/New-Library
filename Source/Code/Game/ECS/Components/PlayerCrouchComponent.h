#pragma once
#include "Engine/ECS/Reflect.h"

struct PlayerCrouchComponent
{
	REFLECT_BEGIN(PlayerCrouchComponent, "Player Crouch")

		REFLECT_FIELD(float, walkSpeed,      (1.2f), ("Walk Speed",      0.0f, 10.0f)) // ‚µ‚á‚ª‚İ•à‚«‚Ì‘¬‚³
		REFLECT_FIELD(float, accelHalfLife, (0.08f), ("Accel Half Life", 0.0f, 1.0f))  // ‰Á‘¬‚Ì”¼Œ¸Šú
		REFLECT_FIELD(float, decelHalfLife, (0.08f), ("Decel Half Life", 0.0f, 1.0f))  // Œ¸‘¬‚Ì”¼Œ¸Šú
		REFLECT_FIELD(float, turnSpeed,      (8.0f), ("Turn Speed",      0.0f, 30.0f)) // ù‰ñ‘¬“x
		REFLECT_FIELD(float, heightScale, (0.5f), ("Height Scale", 0.1f, 1.0f))        // ‚µ‚á‚ª‚İ’†‚Ì“–‚½‚è”»’è‚Ì‚‚³
		
		// ó‘Ô
		REFLECT_FIELD(bool, wantsStand, (false), ("Wants Stand", 0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // —§‚¿‚½‚¢
		REFLECT_FIELD(bool, standRequested, (false), ("Stand Requested", 0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // —§‚¿‚½‚¢ ‚©‚Â —§‚Ä‚é

		REFLECT_END()
};