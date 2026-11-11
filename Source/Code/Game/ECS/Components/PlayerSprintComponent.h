#pragma once
#include "Engine/ECS/Reflect.h"

struct PlayerSprintComponent
{
	REFLECT_BEGIN(PlayerSprintComponent, "Player Sprint")

		REFLECT_FIELD(float, sprintSpeed,   (7.4f),  ("Sprint Speed",    0.0f, 20.0f)) // ‘–‚é‘¬‚³
		REFLECT_FIELD(float, enterSpeed,    (6.0f),  ("Enter Speed",     0.0f, 20.0f)) // ’…’n‚Ì‘¬‚³‚ª‚±‚êˆÈã‚È‚ç Sprint ‚É“ü‚é
		REFLECT_FIELD(float, accelHalfLife, (0.1f),  ("Accel Half Life", 0.0f, 1.0f))  // ‰Á‘¬‚Ì”¼Œ¸Šú
		REFLECT_FIELD(float, decelHalfLife, (0.15f), ("Decel Half Life", 0.0f, 1.0f))  // Œ¸‘¬‚Ì”¼Œ¸Šú
		REFLECT_FIELD(float, turnSpeed,     (8.0f),  ("Turn Speed",      0.0f, 30.0f)) // ù‰ñ‘¬“x
		REFLECT_FIELD(float, enterRunTime,  (1.0f),  ("Enter Run Time", 0.0f, 5.0f))   // Run‚©‚çSprint‚É“ü‚éŒo‰ßŠÔ
	REFLECT_END()
};