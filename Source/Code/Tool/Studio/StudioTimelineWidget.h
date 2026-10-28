#pragma once
#include <imgui.h>

class StudioTimelineWidget
{
public:
	// 目盛りを描いて、ピンのドラッグを処理する
	bool Begin(const char* id, float clipLength, float frameStep, float& inOutTime);
	// トラックを描き終えたら呼ぶ
	void End();

	// 再生ボタンと現在時刻
	static bool DrawToolbar(bool& isPlaying, float time, float frameStep);
	// フレームに吸い付かせる
	static float SnapToFrame(float t, float frameStep, float clipLength);

	// 範囲の端のピン
	bool DrawRangeHandles(const char* id, float handleY, float frameStep, float minLength, float& inOutStart, float& inOutEnd);
	static constexpr float RangeHandleHeight = 16.0f; // 範囲の端のピンの頭の高さ

	// 時刻とX座標の変換
	float TimeToX(float t) const;
	float XToTime(float x) const;

	// トラックを描ける左右の端
	float GetLeft() const { return left; }
	float GetRight() const { return left + width; }

private:
	static constexpr float PinHeadHeight = 16.0f; // 目盛りの上に、ピンの頭を置く余白
	static constexpr float RulerHeight = 24.0f;   // 目盛りの帯の高さ
	static constexpr float LeftPadding = 12.0f;   // 左の余白
	static constexpr float RightPadding = 28.0f;  // 右の余白

	float left = 0.0f;   // 0秒の位置
	float width = 1.0f;  // 0秒から最後までの幅
	float top = 0.0f;    // 目盛りの上端
	float length = 1.0f; // クリップの長さ
	float time = 0.0f;   // ピンの時刻

	// 目盛り
	void DrawRuler(ImDrawList* drawList, float frameStep) const;
	// ピンの頭
	void DrawPinHead(ImDrawList* drawList) const;
	void DrawRangeHandle(ImDrawList* drawList, float x, float handleY, bool highlighted) const;

	// 範囲の端のピンをつかめる幅の半分
	static constexpr float RangeHandleHalfWidth = 7.0f;
};