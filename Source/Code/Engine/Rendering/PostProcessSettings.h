#pragma once
#include <DirectXMath.h>

// ポストエフェクトの設定値
struct PostProcessSettings
{
	// トーンマップ
	float exposure = 1.0f;       // 露出（明るさの倍率。1.0 = そのまま）
	bool  tonemapEnabled = true; // ACES トーンマップを使うか

	// カラー調整
	float contrast = 1.0f;   // コントラスト
	float saturation = 1.0f; // 彩度
	DirectX::XMFLOAT3 colorFilter = { 1.0f, 1.0f, 1.0f }; // カラーフィルタ

};