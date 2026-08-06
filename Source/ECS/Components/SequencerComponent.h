#pragma once

// フラグの種類
enum class SequencerFlag
{
	AnimExit,
	Attack,

	// ここに追加していく

};

struct SequencerComponent
{
	std::string sequencerPath; // .sequencerファイルのパス

	bool animExit = false;
	bool attack = false;
	
	// 種類が増えたらここにも追加する


	float previousNormalizedTime = 0.0f; // 前フレームの正規化時間

};