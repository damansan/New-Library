#pragma once
#include <DirectXMath.h>
#include "Engine/ECS/Reflect.h"

struct MovementComponent
{
	REFLECT_BEGIN(MovementComponent, "Movement")

		// İ’è’l
		REFLECT_FIELD(float,             gravity,     (-20.0f),           ("Gravity",   -100.0f, 0.0f         ))  
		REFLECT_FIELD(DirectX::XMFLOAT3, velocity,    (0.0f, 0.0f, 0.0f), ("Velocity",   0.0f,   0.0f,  0.01f ))
		REFLECT_FIELD(bool,              isGrounded,  (false),            ("Is Grounded"                      ))
		REFLECT_FIELD(float,             planarSpeed, (0.0f),             ("Planar Speed", 0.0f, 0.0f, 0.01f, "", true))

	REFLECT_END()


	// ˆÚ“®‚Ì§ŒäŒ 
    bool movementLocked = false;
    // Œü‚«‚Ì§ŒäŒ 
    bool facingLocked = false;

    // XZ•ûŒü‚Ì§Œä‚ğ‰¡æ‚è‚µ‚Ä‚¢‚éÅ’†‚©
    bool xzControlLocked = false;  
};