#pragma once

// ポストエフェクトの設定値


struct PostProcessSettings
{
	// トーンマップ
	float exposure = 1.0f;       // 露出（明るさの倍率。1.0 = そのまま）
	bool  tonemapEnabled = true; // ACES トーンマップを使うか
};