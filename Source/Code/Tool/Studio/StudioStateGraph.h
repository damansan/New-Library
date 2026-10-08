#pragma once
#include <string>
#include <memory>
#include <vector>

#include "IStudioAttachment.h"
#include "Engine/Data/StateMachineData.h"
#include "Tool/NodeEditor/NodeEditorCanvas.h"

#include "StudioStateParentCanvas.h"
#include "StudioStateChildCanvas.h"

// Stateグラフの編集パネル
class StudioStateGraph : public IStudioAttachment
{
public:
	void Initialize();
	void SetWindowId(int id) { windowId = id; }
	void DrawGUI(entt::registry& registry, StudioContext& context) override;

	// モデルが変わったときに呼ぶ
	void OnModelChanged(StudioContext& context);

private:
	// --- ファイル関係 --- //

	// ツールバー
	void DrawToolbar(StudioContext& context);
	// .stategraphのドロップ受け口
	void DrawDropTarget(StudioContext& context);
	// ドロップされたファイルが開けるか確認
	void TryOpen(const std::string& path, StudioContext& context);

	// 保存 / 読み込み
	void Save(StudioContext& context);
	void Load(StudioContext& context);

	// 保存先のパスを決める
	std::string BuildNewSavePath(const StudioContext& context) const;

	// アニメーションを再生できるモデルか
	static bool HasAnimatedModel(entt::registry& registry);


	// --- データ --- //

	StateMachineData data;

	// --- パネルの状態 --- //

	int windowId = 0;

	// attachedGraphPathの内容を読み込み済みか
	bool loaded = false;
	// 最後の検証でエラーがあったか
	bool hasErrors = false;
	// ツールバーに出すメッセージ
	std::string message;

	bool columnWidthInitialized = false;


	// --- キャンバス --- //

	// 親階層キャンバス
	StudioStateParentCanvas parentCanvas;
	// 子階層キャンバス(複数)
	std::vector<std::unique_ptr<StudioStateChildCanvas>> childCanvases;


	// --- 子階層のキャンバス --- //

	// 次に手前に出す子のウィンドウ
	int focusChildStateId = -1;

	// 子のウィンドウを開く
	void OpenChildWindow(int stateId);
	// 開いている子のウィンドウを描く
	void DrawChildWindows();
};