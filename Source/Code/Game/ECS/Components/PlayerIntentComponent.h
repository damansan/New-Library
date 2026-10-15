#pragma once
#include <DirectXMath.h>
#include "Engine/ECS/Reflect.h"

// プレイヤーの意図
struct PlayerIntentComponent
{
	REFLECT_BEGIN(PlayerIntentComponent, "Player Intent")

		// 設定値
		REFLECT_FIELD(float, moveGraceTime,     (0.1f), ("Move Grace Time",     0.0f, 0.5f)) // 入力が途切れても wantsMove を保つ時間
		REFLECT_FIELD(float, runEnterThreshold, (0.9f), ("Run Enter Threshold", 0.0f, 1.0f)) // この倒し具合以上で走りに入る
		REFLECT_FIELD(float, runExitThreshold,  (0.7f), ("Run Exit Threshold",  0.0f, 1.0f)) // この倒し具合未満で歩きに戻る

		// 意図
		REFLECT_FIELD(bool, wantsMove, (false), ("Wants Move", 0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 動きたいか(猶予付き)
		REFLECT_FIELD(bool, wantsRun,  (false), ("Wants Run",  0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 走りたいか(ヒステリシス付き)
		
		REFLECT_FIELD(DirectX::XMFLOAT3, moveDir, (0.0f, 0.0f, 0.0f), ("Move Dir", 0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // ワールド方向(長さ1)。猶予中は最後の方向を保つ
		
		REFLECT_FIELD(float, moveAmount,  (0.0f),  ("Move Amount",   0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 倒し具合(0～1)
		REFLECT_FIELD(bool,  hasRawInput, (false), ("Has Raw Input", 0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 今フレームの生の入力があるか(確認用)

		REFLECT_END()

		// 内部実装用
		float graceTimer = 0.0f; // 入力が途切れてからの経過時間
};