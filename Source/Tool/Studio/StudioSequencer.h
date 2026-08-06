#pragma once
#include "IStudioAttachment.h"
#include "Tool/SequencerData.h"
#include <imgui.h>
#include <functional>
#include <string>
#include <vector>

class StudioSequencer : public IStudioAttachment
{
public:
	StudioSequencer() = default;
	~StudioSequencer() = default;

	void SetWindowId(int id) { windowId = id; }

	// ImGui描画
	void DrawGUI(entt::registry& registry, StudioContext& context) override;

private:
	// == 内部状態 == //
	int windowId = 0;

	SequencerData sequencerData;   // 編集中のSequencerデータ
	std::string selectedClipName;  // 今編集しているクリップ名
	std::vector<std::string> availableAnimations; // モデルが持つアニメーション名一覧

	enum class SelectionType { None, Range, Point };
	SelectionType selectedType = SelectionType::None;
	int selectedId = -1; // 選択中のRange/PointのID
	std::string loadedModelPath; // 最後に読み込んだモデルパス

	// == 各トラックの描画 == //
	void DrawToolbar(SequencerClipData& clip, float previewTime); // ツールバー
	void DrawPointTrack(SequencerClipData& clip, float previewTime, float clipLength); // Pointのトラック描画
	void DrawRangeTrack(SequencerClipData& clip, float previewTime, float clipLength); // Rangeのトラック描画
	void DrawDetailPanel(SequencerClipData& clip, const std::string& modelPath); // 詳細パネル

	// == 削除 == //
	void DeleteRange(SequencerClipData& clip, int id); // Range削除
	void DeletePoint(SequencerClipData& clip, int id); // Point削除
	void DrawDeleteButton(const char* label, const std::function<void()>& onDelete); // 削除ボタン

	// == 座標変換ヘルパー == //
	static float TimeToNormalized(float time, float clipLength);       // 秒数を0.0～1.0に
	static float NormalizedToTime(float normalized, float clipLength); // 0.0～1.0を秒数に

	// == 保存 == //
	void Save(const std::string& modelPath); // 保存
	static std::string GetSequencerFilePath(const std::string& modelPath); // モデルパスから保存先のパスを組み立てる

	// == その他 == //
	void RefreshAvailableAnimations(entt::registry& registry);   // アニメーション一覧を更新する
	static const char* GetSequencerFlagName(SequencerFlag flag); // シーケンサーフラグの表示名を返す
}; 