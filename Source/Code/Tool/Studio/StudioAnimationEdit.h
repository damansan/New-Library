#pragma once
#include "IStudioAttachment.h"
#include "Engine/Data/AnimationEditData.h"
#include <imgui.h>
#include <string>

class Model;
struct SkinnedMeshComponent;
struct AnimatorComponent;

class StudioAnimationEdit : public IStudioAttachment
{
public:
	StudioAnimationEdit() = default;
	~StudioAnimationEdit() = default;

	void SetWindowId(int id) { windowId = id; }

	// ImGui描画
	void DrawGUI(entt::registry& registry, StudioContext& context) override;

private:
	// 内部状態
	int windowId = 0;

	AnimationEditData editData;   // 編集中の設定
	std::string loadedModelPath;  // 最後に読み込んだモデルパス
	bool isDirty = false;         // 未保存の変更があるか

	// 各パネル
	void DrawInspectorPanel(SkinnedMeshComponent* skinned, AnimatorComponent* animator, StudioContext& context); // パネル1：数値
	void DrawTimelinePanel(SkinnedMeshComponent* skinned, AnimatorComponent* animator, StudioContext& context);  // パネル2：タイムライン
	static bool DrawNotReadyMessage(SkinnedMeshComponent* skinned, AnimatorComponent* animator);                 // 表示できないとき

	// 各項目 
	bool DrawClipSettings(const std::string& clipName); // クリップ設定（変更があればtrue）

	// 編集後の処理
	void OnEdited(SkinnedMeshComponent& skinned, StudioContext& context); // 焼き直し・プレビュー更新・未保存フラグ

	// クリップ設定
	bool DrawClipSettings(const std::string& clipName);       
	// 焼き込み後のルート移動量
	void DrawRootMotionInfo(const Model& model, const std::string& clipName); 


	// 保存読み込み
	void Load(const std::string& modelPath, const Model& model);
	void Save(const std::string& modelPath);
};