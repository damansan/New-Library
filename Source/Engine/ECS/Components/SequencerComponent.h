#pragma once
#include <string>

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
	std::string sequencerPath; // .sequencerファイルのパス

	bool animExit = false;
	bool superArmor = false;
	bool attack = false;
	// 種類が増えたらここにも追加する


	float previousNormalizedTime = 0.0f; // 前フレームの正規化時間
};