#pragma once
#include <string>
#include "Engine/ECS/Reflect.h"

// フラグの種類
enum class SequencerFlag
{
	AnimExit,   // アニメーション終了フラグ
	SuperArmor, // 発生保障フラグ
	Attack,     // 攻撃フラグ

	Trimming,   // アニメーション再生範囲

	// 種類が増えたらここに追加していく

	Count
};

struct SequencerComponent
{
	REFLECT_BEGIN(SequencerComponent, "Sequencer")

		// ファイルパス
		REFLECT_FIELD(std::string, sequencerPath, (""), ("Sequencer File", 0.0f, 0.0f, 0.1f, "SEQUENCER_PATH")) // .sequencerファイルのパス

		// 各フラグ
		REFLECT_FIELD(bool, animExit,   (false), ("Anim Exit",  0.0f, 0.0f, 0.1f, "", true))
		REFLECT_FIELD(bool, superArmor, (false), ("SuperArmor", 0.0f, 0.0f, 0.1f, "", true))
		REFLECT_FIELD(bool, attack,     (false), ("Attack",     0.0f, 0.0f, 0.1f, "", true))

	REFLECT_END()

	float previousNormalizedTime = 0.0f; // 前フレームの正規化時間
};