#pragma once
#include <DirectXMath.h>
#include "Engine/ECS/Reflect.h"


// ‰‰oó‘Ô
struct PulseState
{
	REFLECT_BEGIN(PulseState, "Pulse State")

		REFLECT_FIELD(bool, active, (false), ("Active", 0.0f, 0.0f, 0.1f,  "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // Ä¶’†
		REFLECT_FIELD(bool, ready,  (true),  ("Ready",  0.0f, 0.0f, 0.1f,  "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // €”õŠ®—¹
		REFLECT_FIELD(float, time,  (0.0f),  ("Time",   0.0f, 0.0f, 0.01f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // Œo‰ßŠÔ

	REFLECT_END()
};

// ƒJƒƒ‰‚Ì‰‰o
struct CameraEffectComponent
{
	REFLECT_BEGIN(CameraEffectComponent, "Camera Effect")

		// ”­‰ÎğŒ
		REFLECT_FIELD(float, runSpeed, (5.0f), ("Run Speed", 0.0f, 50.0f)) // ‚±‚Ì‘¬‚³ˆÈã‚ğ‘–‚è‚Æ‚·‚é

		// FovF‘¬‚³‚ÅL‚°‚é
		REFLECT_FIELD(float, fovAddMax,   (15.0f), ("Fov Add Max", 0.0f, 60.0f))   // ‘¬‚¢‚É‘«‚· FOV
		REFLECT_FIELD(float, speedFull,   (15.0f), ("Speed Full", 0.0f, 50.0f))    // ‚±‚Ì‘¬‚³‚Å FOV ‚ğ‘«‚·
		REFLECT_FIELD(float, fovHalfLife, (0.25f), ("Fov Half Life", 0.01f, 2.0f)) // ‘«‚·—Ê‚ğŠñ‚¹‚é”¼Œ¸Šú

		// HeadBobF‘–‚èo‚µ‚É¶‰E‚Ö—h‚ç‚·
		REFLECT_FIELD(float, runBobRearmSpeed,   (1.0f), ("Run Bob Rearm Speed",   0.0f, 50.0f)) // ‚±‚Ì‘¬‚³ˆÈ‰º‚Å€”õ
		REFLECT_FIELD(float, runBobStepInterval, (0.3f), ("Run Bob Step Interval", 0.05f, 1.0f)) // •Ğ‘¤‚Ö—h‚ê‚éŠÔ
		REFLECT_FIELD(float, runBobDuration,     (1.5f), ("Run Bob Duration",      0.1f, 5.0f))  // —h‚ç‚·ŠÔ
		REFLECT_FIELD(float, runBobSway,        (0.15f), ("Run Bob Sway",          0.0f, 1.0f))  // —h‚ê•(‰¡)
		REFLECT_FIELD(float, runBobLift,        (0.05f), ("Run Bob Lift",          0.0f, 1.0f))  // —h‚ê•(c)
		REFLECT_FIELD(float, runBobRollDeg,      (2.0f), ("Run Bob Roll Deg",      0.0f, 15.0f)) // ŒX‚«‚ÌÅ‘å

		// KickShakeF•ÇR‚è‚Å1‰ñ—h‚ç‚·
		REFLECT_FIELD(float, kickShakeDuration, (0.3f), ("Kick Shake Duration", 0.05f, 2.0f))  // Šñ‚Á‚Ä–ß‚é‚Ü‚Å‚ÌŠÔ
		REFLECT_FIELD(float, kickShakeSway,    (0.08f), ("Kick Shake Sway",     0.0f, 0.5f))   // ‰¡‚¸‚ê‚ÌÅ‘å
		REFLECT_FIELD(float, kickShakeRollDeg,  (1.5f), ("Kick Shake Roll Deg", 0.0f, 10.0f))  // ŒX‚«‚ÌÅ‘å

		// LandShakeF’·‚­—‰º‚µ‚½‚ç—h‚ç‚·
		REFLECT_FIELD(float, landFallSpeed,    (10.0f), ("Land Fall Speed",     0.0f, 50.0f))  // ‚±‚Ì—‰º‘¬“xˆÈã‚Å€”õ
		REFLECT_FIELD(float, landShakeDuration, (0.5f), ("Land Shake Duration", 0.05f, 2.0f))  // —h‚ç‚·ŠÔ
		REFLECT_FIELD(float, landShakeSway,    (0.12f), ("Land Shake Sway",     0.0f, 0.5f))   // —h‚ê•
		REFLECT_FIELD(float, landShakeRollDeg,  (2.0f), ("Land Shake Roll Deg", 0.0f, 10.0f))  // ŒX‚«

		// SlideShakeFƒXƒ‰ƒCƒh’†‚Í‰º‚°‚é
		REFLECT_FIELD(float, slideDropTime, (0.525f), ("Slide Drop Time", 0.01f, 2.0f)) // ’ê‚Ü‚Å‰º‚ª‚é(–ß‚é)ŠÔ
		REFLECT_FIELD(float, slideDrop,       (0.5f), ("Slide Drop",      0.0f, 3.0f))  // ‰º‚°‚é[‚³

		// WallTiltF•Ç‘–‚è’†ŒX‚¯‚é
		REFLECT_FIELD(float, wallTiltMaxDeg,       (8.0f), ("Wall Tilt Max Deg",      0.0f, 30.0f))  // ŒX‚«‚ÌÅ‘å(“x)
		REFLECT_FIELD(float, wallTiltHalfLife,     (1.0f), ("Wall Tilt Half Life",    0.01f, 5.0f))  // ŒX‚¢‚Ä‚¢‚­”¼Œ¸Šú(•b)B‘å‚«‚¢‚Ù‚Ç‚¶‚í‚¶‚í
		REFLECT_FIELD(float, wallTiltReturnSpeed, (40.0f), ("Wall Tilt Return Speed", 0.0f, 200.0f)) // –ß‚é‘¬‚³


		// ‰‰o‚²‚Æ‚Ìó‘Ô
		REFLECT_FIELD(float, fovAdd,        (0.0f), ("Fov Add",         0.0f, 0.0f, 0.1f,  "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // FovFŒ»İ’Ç‰Á‚µ‚Ä‚¢‚é—Ê
		REFLECT_FIELD(float, slideDropRate, (0.0f), ("Slide Drop Rate", 0.0f, 0.0f, 0.01f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // SlideDropF‰º‚ª‚è‹ï‡
		REFLECT_FIELD(float, wallTilt,      (0.0f), ("Wall Tilt",       0.0f, 0.0f, 0.1f,  "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // WallTiltF¡‚ÌŒX‚«

		REFLECT_FIELD(PulseState, runBob,    (), ("Run Bob"))    // HeadBob
		REFLECT_FIELD(PulseState, kickShake, (), ("Kick Shake")) // KickShake
		REFLECT_FIELD(PulseState, landShake, (), ("Land Shake")) // LandShake


		// o—Í
		REFLECT_FIELD(DirectX::XMFLOAT3, eyeOffset,    (0.0f, 0.0f, 0.0f), ("Eye Offset",    0.0f, 0.0f, 0.01f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // eye ‚Ì‚¸‚ê
		REFLECT_FIELD(DirectX::XMFLOAT3, targetOffset, (0.0f, 0.0f, 0.0f), ("Target Offset", 0.0f, 0.0f, 0.01f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // target ‚Ì‚¸‚ê
		REFLECT_FIELD(float, fovOffset, (0.0f), ("Fov Offset", 0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // FOV ‚Ì‘Œ¸
		REFLECT_FIELD(float, roll,      (0.0f), ("Roll",       0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // ŒX‚«


		REFLECT_END()
};