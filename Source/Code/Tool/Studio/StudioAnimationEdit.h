#pragma once
#include "IStudioAttachment.h"
#include "Engine/Data/AnimationEditData.h"
#include "StudioTimelineWidget.h"
#include <imgui.h>
#include <string>
#include <vector>

class Model;
struct AnimatorComponent;

// ルートモーションのグラフ用の値
struct RootMotionCurve
{
	std::vector<float> times; // 時刻
	std::vector<float> xz;    // 水平に進んだ距離の累計
	std::vector<float> y;     // 開始時からの高さ
};

class StudioAnimationEdit : public IStudioAttachment
{
public:
	StudioAnimationEdit() = default;
	~StudioAnimationEdit() = default;

	void SetWindowId(int id) { windowId = id; }

	// ImGui描画
	void DrawGUI(entt::registry& registry, StudioContext& context) override;

private:
	// 編集モード
	enum class EditMode
	{
		Trim,       // トリミング
		RootMotion, // ルートモーション
	};

	// 内部状態
	int windowId = 0;

	AnimationEditData editData;      // 編集中の設定
	std::string loadedModelPath;     // 最後に読み込んだモデルパス
	bool isDirty = false;            // 未保存の変更があるか
	EditMode mode = EditMode::Trim;  // 今のモード
	StudioTimelineWidget timeline;   // タイムライン部品
	bool bakedUntrimmed = false;     // トリミングなしで焼かれているか
	bool rangePlaying = false;       // トリミングモードで、使う範囲を再生中か

	RootMotionCurve originalCurve;   // 区間を外したもの
	RootMotionCurve editedCurve;     // 編集後
	std::string curveClipName;       // 今のグラフがどのクリップのものか
	bool curveDirty = true;          // グラフを作り直す必要があるか

	// タイムライン
	bool DrawTimelineArea(const Model& model, AnimatorComponent& animator, StudioContext& context);
	// 数値
	bool DrawDetailArea(const Model& model, const std::string& clipName, const std::string& modelPath);


	// トリミングのトラック
	bool DrawTrimTrack(const std::string& clipName, float rawLength, float frameStep);
	// トリミングのボタン
	bool DrawTrimButtons(const std::string& clipName, float rawLength, float frameStep, float pinTime); 


	// ルートモーションのグラフ
	void DrawRootMotionTracks(const Model& model, const std::string& clipName);
	// グラフの値を作り直す
	void RebuildCurves(const Model& model, const std::string& clipName);
	// グラフのトラックを1本描く
	void DrawCurveTrack(const char* label, bool isY, float height, ImU32 color);


	// クリップ設定
	bool DrawClipSettings(const Model& model, const std::string& clipName); 


	// 使う範囲の中で時刻を進める
	void AdvanceRangePlayback(AnimatorComponent& animator, StudioContext& context, float start, float end); 
	// 範囲を取り出す
	void GetTrimRange(const std::string& clipName, float rawLength, float& outStart, float& outEnd) const;
	// 範囲を書き込む
	bool SetTrimRange(const std::string& clipName, float rawLength, float start, float end);            


	// 1フレームの秒数
	static float CalcFrameStep(const Model& model, const std::string& clipName); 
	// 生データの長さ
	static float FindRawLength(const Model& model, const std::string& clipName); 


	// モードに合わせて焼き直す
	void Rebake(Model& model) const;  
	// モードと焼き方がずれていたら合わせる
	void SyncBakeWithMode(Model& model, AnimatorComponent& animator, StudioContext& context); 


	// 保存読み込み
	void Load(const std::string& modelPath, const Model& model);
	void Save(const std::string& modelPath);
};