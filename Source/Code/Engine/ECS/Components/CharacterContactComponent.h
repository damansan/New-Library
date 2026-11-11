#pragma once
#pragma once
#include <DirectXMath.h>
#include "Engine/ECS/Reflect.h"

// キャラクターが触れた面の結果
struct CharacterContactComponent
{
	REFLECT_BEGIN(CharacterContactComponent, "Character Contact")

		REFLECT_FIELD(bool, hitSide, (false), ("Hit Side", 0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // このフレームの 移動 で横に当たったか
		REFLECT_FIELD(DirectX::XMFLOAT3, sideNormal, (0.0f, 0.0f, 0.0f), ("Side Normal", 0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 横に当たった面の法線

		REFLECT_END()
};