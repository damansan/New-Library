#pragma once
#include "IStudioAttachment.h"

#include "Engine/Core/JsonFileCache.h"
#include "Engine/Data/AnimationEditData.h"
#include "Engine/Resource/Model.h"

#include "StudioTimelineWidget.h"

#include <imgui.h>
#include <string>
#include <vector>

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
	// ウィンドウを閉じている間に呼ぶ
	void OnHidden(entt::registry& registry, StudioContext& context);

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
	JsonFileCache<AnimationEditData> fileCache; // ファイルの読み書き
	std::string loadedModelPath;     // 最後に読み込んだモデルパス
	bool isDirty = false;            // 未保存の変更があるか
	EditMode mode = EditMode::Trim;  // 今のモード
	StudioTimelineWidget timeline;   // タイムライン部品
	bool previewActive = false;      // トリミングのプレビュー
	bool rangePlaying = false;       // トリミングモードで、使う範囲を再生中か

	// 区間の軸
	enum class SectionAxis
	{
		XZ, // 水平移動
		Y,  // 上下移動
	};
	SectionAxis viewAxis = SectionAxis::XZ;     // 表示している軸
	SectionAxis selectedAxis = SectionAxis::XZ; // 選んでいる区間の軸
	int selectedSection = -1;                   // 選んでいる区間の番号

	RootMotionCurve originalCurve; // 区間を外したもの
	RootMotionCurve editedCurve;   // 編集後
	std::string curveClipName;     // 今のグラフがどのクリップのものか
	bool curveDirty = true;        // グラフを作り直す必要があるか



	// --- 全体 --- //

	// モードの切り替え
	void DrawModeBar();
	// タイムライン
	bool DrawTimelineArea(const Model& model, AnimatorComponent& animator, StudioContext& context);
	// 数値
	bool DrawDetailArea(const Model& model, const std::string& clipName, const std::string& modelPath);
	// Root Node の選択
	bool DrawRootNodeCombo(const Model& model);
	// クリップ設定（モードに合わせて、トリミングか区間の数値を出す）
	bool DrawClipSettings(const Model& model, const std::string& clipName);

	// 1フレームの秒数
	static float CalcFrameStep(const Model& model, const std::string& clipName);
	// 生データの長さ
	static float FindRawLength(const Model& model, const std::string& clipName);

	// モードに合わせて焼き直す
	void Rebake(Model& model) const;
	// モードと焼き方がずれていたら合わせる
	void SyncBakeWithMode(Model& model, AnimatorComponent& animator, StudioContext& context);
	// トリミングのプレビューを使う／やめる
	void SetPreviewActive(Model& model, AnimatorComponent& animator, StudioContext& context, bool active);

	// 保存読み込み
	void Load(const std::string& modelPath, const Model& model);
	void Save(const std::string& modelPath);



	// --- トリミングモード --- //

	// トリミングのトラック
	bool DrawTrimTrack(const std::string& clipName, float rawLength, float frameStep);
	// トリミングのボタン
	bool DrawTrimButtons(const std::string& clipName, float rawLength, float frameStep, float pinTime);
	// トリミングの数値
	bool DrawTrimSettings(const Model& model, AnimationEditClipData& clip);

	// 使う範囲の中で時刻を進める
	void AdvanceRangePlayback(AnimatorComponent& animator, StudioContext& context, float start, float end);
	// 範囲を取り出す
	void GetTrimRange(const std::string& clipName, float rawLength, float& outStart, float& outEnd) const;
	// 範囲を書き込む
	bool SetTrimRange(const std::string& clipName, float rawLength, float start, float end);



	// --- ルートモーションモード --- //

	// ルートモーションのグラフ
	bool DrawRootMotionTracks(const Model& model, const Model::Animation& animation, const std::string& clipName, float frameStep);
	// グラフの値を作り直す
	void RebuildCurves(const Model& model, const std::string& clipName);
	// グラフのトラックを1本描く
	float DrawCurveTrack(const char* label, bool isY, float height, ImU32 color, float tintStart, float tintEnd);
	// 区間の帯を描いて、クリックで選択する
	bool DrawSectionBand(const char* id, SectionAxis axis, const Model::Animation& animation, const std::string& clipName, float frameStep, float graphTop);
	// 区間の追加とSet Start / Set End
	bool DrawSectionButtons(const Model::Animation& animation, const std::string& clipName, float frameStep, float pinTime);
	// 選んでいる区間の数値
	bool DrawSectionSettings(const Model& model, AnimationEditClipData& clip);

	// 選んでいる区間の範囲
	bool GetSelectedSectionRange(SectionAxis axis, const Model::Animation& animation, const std::string& clipName, float& outStart, float& outEnd) const;
	// 区間が動ける範囲
	static void GetSectionLimits(const std::vector<AnimationEditSection>& sections, int index, const Model::Animation& animation, float& outLower, float& outUpper); 
};