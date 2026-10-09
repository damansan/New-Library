#include "StudioStateGraph.h"
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <imgui.h>
#include <json.hpp>
#include <cmath>
#include <cfloat>
#include <memory>

#include "Engine/ECS/Components/SkinnedMeshComponent.h"
#include "Engine/ECS/Components/AnimatorComponent.h"

#include "Tool/Editor/ProjectWindow.h"

void StudioStateGraph::Initialize()
{
	// 親階層キャンバス初期化
	parentCanvas.Initialize();
	// 条件のキャンバス初期化
	conditionGraph.Initialize();
}

void StudioStateGraph::DrawGUI(entt::registry& registry, StudioContext& context)
{
	// 子のウィンドウに渡すアニメーションの一覧
	RefreshAnimationNames(registry);

	// 紐づいたファイルが変わったら読み込む
	if (!loaded && !context.attachedGraphPath.empty())
	{
		Load(context);
		loaded = true;
	}

	std::string panelName = "StateGraph##" + std::to_string(windowId);
	bool open = ImGui::Begin(panelName.c_str());

	// StaticModelは弾く
	if (open && !HasAnimatedModel(registry))
	{
		ImGui::Text("No skinned model.");
	}
	else if (open)
	{
		DrawToolbar(context);

		ImGui::Columns(2, "StateGraphColumns", true);
		if (!columnWidthInitialized)
		{
			ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * 0.75f);
			columnWidthInitialized = true;
		}

		// 左：グラフ
		parentCanvas.Draw(data);
		DrawDropTarget(context);

		ImGui::NextColumn();

		// 右：インスペクター
		parentCanvas.DrawInspector(data);

		// 選択している物の削除(文字を入力中は、Deleteキーを文字の削除に使うので除く)
		if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
			!ImGui::GetIO().WantTextInput &&
			ImGui::IsKeyPressed(ImGuiKey_Delete))
		{
			parentCanvas.DeleteSelection(data);
		}

		ImGui::Columns(1);
		
		// 親階層から子階層のキャンバスを開く依頼があれば開く
		int openRequest = parentCanvas.ConsumeOpenChildRequest();
		if (openRequest != -1) OpenChildWindow(openRequest);

		// 親のリンクの条件を編集する依頼があれば開く
		ConditionTarget conditionRequest = parentCanvas.ConsumeEditConditionRequest();
		if (conditionRequest.IsValid()) OpenConditionWindow(conditionRequest);
	}

	ImGui::End();

	// 子階層ウィンドウ
	DrawChildWindows();
	// 条件のウィンドウ
	DrawConditionWindow();
}

// モデルが変わったときに呼ぶ
void StudioStateGraph::OnModelChanged(StudioContext& context)
{
	data.Clear();

	// 選択などを戻す
	parentCanvas.Reset();
	childCanvases.clear();
	CloseConditionWindow();

	context.attachedGraphPath.clear();
	loaded = true;     // 空のグラフを、読み込み済みとして扱う
	hasErrors = false; // 作り始めなので、検証は保存時に行う
	message.clear();
}

// ツールバー
void StudioStateGraph::DrawToolbar(StudioContext& context)
{
	if (ImGui::Button("Save"))
	{
		Save(context);
	}
	ImGui::SameLine();
	if (!context.attachedGraphPath.empty())
	{
		ImGui::TextDisabled("%s", context.attachedGraphPath.c_str());
	}
	else
	{
		ImGui::TextDisabled("Not saved yet.");
	}

	// ファイルの読み書きの失敗など
	if (!message.empty())
	{
		ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", message.c_str());
	}
	// 検証エラー
	if (hasErrors)
	{
		ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "Validation errors. See Log.");
	}

	ImGui::Separator();
}

// .stategraphのドロップ受け口
void StudioStateGraph::DrawDropTarget(StudioContext& context)
{
	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("STATEGRAPH_PATH"))
		{
			std::string path(static_cast<const char*>(payload->Data));
			TryOpen(path, context);
		}
		ImGui::EndDragDropTarget();
	}
}

// ドロップされたファイルが開けるか確認
void StudioStateGraph::TryOpen(const std::string& path, StudioContext& context)
{
	std::ifstream ifs(path);
	if (!ifs.is_open())
	{
		message = "Failed to open file.";
		return;
	}

	std::string sourceModelPath;
	try
	{
		nlohmann::json j;
		ifs >> j;
		sourceModelPath = j.value("sourceModelPath", "");
	}
	catch (const std::exception&)
	{
		message = "Invalid json.";
		return;
	}

	// 別のモデル用のグラフは開かない
	if (!sourceModelPath.empty() && sourceModelPath != context.currentModelPath)
	{
		message = "This graph belongs to a different model.";
		return;
	}

	message.clear();
	context.attachedGraphPath = path;
	loaded = false; // 次のフレームで読み込む
}

// 保存
void StudioStateGraph::Save(StudioContext& context)
{
	// まだ保存先が無ければ 自動で決める
	if (context.attachedGraphPath.empty())
	{
		context.attachedGraphPath = BuildNewSavePath(context);
		loaded = true; // 今のデータがそのまま保存先の内容になるので、読み直さない
	}

	// 作りかけでも保存はできる、エラーの有無はツールバーに表示する
	hasErrors = !data.Validate(context.attachedGraphPath);

	nlohmann::ordered_json j = data.ToJson();
	j["sourceModelPath"] = context.currentModelPath;

	std::ofstream ofs(context.attachedGraphPath);
	if (!ofs.is_open())
	{
		message = "Failed to save file.";
		return;
	}
	ofs << j.dump(4);
	message.clear();
}
// 読み込み
void StudioStateGraph::Load(StudioContext& context)
{
	std::ifstream ifs(context.attachedGraphPath);
	if (!ifs.is_open())
	{
		message = "Failed to open file.";
		return;
	}

	try
	{
		nlohmann::json j;
		ifs >> j;

		// 検証でエラーがあっても、直すために読み込んだ状態で開く
		hasErrors = !data.FromJson(j, context.attachedGraphPath);

		// 選択などを戻す
		parentCanvas.Reset();
		childCanvases.clear();
		CloseConditionWindow();

		message.clear();
	}
	catch (const std::exception&)
	{
		message = "Invalid json.";
	}
}

// 保存先のパスを決める
std::string StudioStateGraph::BuildNewSavePath(const StudioContext& context) const
{
	std::filesystem::path saveDir = "Data/Json/StateGraph";
	std::filesystem::create_directories(saveDir);

	std::string modelName = std::filesystem::path(context.currentModelPath).stem().string();
	if (modelName.empty()) modelName = "NewStateGraph";

	std::string uniqueName = ProjectWindow::GenerateUniqueName(saveDir, modelName + ".stategraph");
	std::string result = (saveDir / uniqueName).string();
	std::replace(result.begin(), result.end(), '\\', '/');
	return result;
}

// モデルが持っているアニメーションの名前の一覧を作り直す
void StudioStateGraph::RefreshAnimationNames(entt::registry& registry)
{
	animationNames.clear();

	auto view = registry.view<SkinnedMeshComponent>();
	if (view.begin() == view.end()) return;

	auto& skinned = view.get<SkinnedMeshComponent>(*view.begin());
	if (!skinned.model) return;

	for (const auto& anim : skinned.model->GetAnimations())
	{
		animationNames.push_back(anim.name);
	}
}

// アニメーションを再生できるモデルか
bool StudioStateGraph::HasAnimatedModel(entt::registry& registry)
{
	auto view = registry.view<SkinnedMeshComponent, AnimatorComponent>();
	return view.begin() != view.end();
}

// 子のウィンドウを開く
void StudioStateGraph::OpenChildWindow(int stateId)
{
	// すでに開いていれば、新しく開かずに手前に出す
	for (const auto& child : childCanvases)
	{
		if (child->GetStateId() == stateId)
		{
			focusChildStateId = stateId;
			return;
		}
	}

	auto child = std::make_unique<StudioStateChildCanvas>(stateId);
	child->Initialize();
	childCanvases.push_back(std::move(child));
	focusChildStateId = stateId;
}

// 開いている子のウィンドウを描く
void StudioStateGraph::DrawChildWindows()
{
	for (auto it = childCanvases.begin(); it != childCanvases.end(); )
	{
		StudioStateChildCanvas& child = **it;

		// Stateが消えていれば閉じる
		const StateNode* state = std::as_const(data).FindState(child.GetStateId());
		if (!state)
		{
			it = childCanvases.erase(it);
			continue;
		}

		// 表示はState名、IDはStateのIDで作る
		std::string title = "Child: " + state->name +
			"###StateChild_" + std::to_string(windowId) + "_" + std::to_string(state->id);

		// タイトルバーをStateの色にする
		const float* c = state->editorColor;
		ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(c[0] * 0.7f, c[1] * 0.7f, c[2] * 0.7f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(c[0], c[1], c[2], 1.0f));

		// 開いた直後だけ大きさを決める
		ImGui::SetNextWindowSize(ImVec2(700.0f, 450.0f), ImGuiCond_FirstUseEver);
		if (focusChildStateId == state->id)
		{
			ImGui::SetNextWindowFocus();
			focusChildStateId = -1;
		}

		bool open = true;
		bool visible = ImGui::Begin(title.c_str(), &open);
		ImGui::PopStyleColor(2);

		if (visible)
		{
			child.Draw(data, animationNames);
		}
		ImGui::End();

		// 子のリンクの条件を編集する依頼があれば開く
		ConditionTarget conditionRequest = child.ConsumeEditConditionRequest();
		if (conditionRequest.IsValid()) OpenConditionWindow(conditionRequest);

		// ×で閉じられたら、一覧から外す
		if (!open) it = childCanvases.erase(it);
		else ++it;
	}
}

// 条件のウィンドウを開く
void StudioStateGraph::OpenConditionWindow(const ConditionTarget& target)
{
	// 別のリンクに切り替えるときは、実体が同じ場所でも必ず画面をリセットする
	if (target != conditionTarget) conditionPointer = nullptr;

	conditionTarget = target;
	focusConditionWindow = true;
}

// 条件のウィンドウを描く
void StudioStateGraph::DrawConditionWindow()
{
	if (!conditionTarget.IsValid()) return;

	// 編集する条件の実体を、毎フレーム探し直す
	std::string description;
	ImVec4 color;
	ConditionGraph* condition = ResolveCondition(conditionTarget, description, color);

	// リンクが消えていたら閉じる
	if (!condition)
	{
		CloseConditionWindow();
		return;
	}

	// 実体の場所が変わったら条件の画面をリセットする
	if (condition != conditionPointer)
	{
		conditionGraph.SetTarget(condition);
		conditionPointer = condition;
	}

	std::string title = "Condition: " + description + "###StateCondition_" + std::to_string(windowId);

	// タイトルバーを、リンクの持ち主のStateの色にする
	ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(color.x * 0.7f, color.y * 0.7f, color.z * 0.7f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_TitleBgActive, color);

	// 開いた直後だけ大きさを決める
	ImGui::SetNextWindowSize(ImVec2(800.0f, 500.0f), ImGuiCond_FirstUseEver);
	if (focusConditionWindow)
	{
		ImGui::SetNextWindowFocus();
		focusConditionWindow = false;
	}

	bool open = true;
	bool visible = ImGui::Begin(title.c_str(), &open);
	ImGui::PopStyleColor(2);

	if (visible)
	{
		// 左：条件のキャンバス、右：条件のインスペクター
		ImGui::Columns(2, "StateConditionColumns", true);
		if (!conditionColumnInitialized)
		{
			ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * 0.7f);
			conditionColumnInitialized = true;
		}

		conditionGraph.DrawGUI();
		ImGui::NextColumn();
		conditionGraph.DrawInspector();

		ImGui::Columns(1);
	}
	ImGui::End();

	// ×で閉じられた
	if (!open) CloseConditionWindow();
}

// 条件のウィンドウを閉じる
void StudioStateGraph::CloseConditionWindow()
{
	conditionTarget = {};
	conditionPointer = nullptr;
	conditionGraph.SetTarget(nullptr);
}

// 編集する条件の実体を探す
ConditionGraph* StudioStateGraph::ResolveCondition(const ConditionTarget& target, std::string& outTitle, ImVec4& outColor)
{
	switch (target.kind)
	{
	case ConditionTarget::Kind::StateLink:
	{
		// 親のリンク：遷移元 → 遷移先
		StateTransition* t = data.FindTransition(target.linkId);
		if (!t) return nullptr;

		std::string fromName;
		if (!FindTransitionSource(t->id, fromName, outColor)) return nullptr;

		const StateNode* to = std::as_const(data).FindState(t->toStateId);
		outTitle = fromName + " -> " + (to ? to->name : "(missing)");
		return &t->condition;
	}

	case ConditionTarget::Kind::EntryLink:
	case ConditionTarget::Kind::ClipLink:
	{
		// 子のリンク：State名 / 元 → 先
		StateNode* state = data.FindState(target.stateId);
		if (!state) return nullptr;

		const float* c = state->editorColor;
		outColor = ImVec4(c[0], c[1], c[2], 1.0f);

		auto clipName = [state](int clipId) -> std::string
			{
				const ClipNode* clip = state->child.FindClip(clipId);
				return clip ? clip->name : "(missing)";
			};

		if (target.kind == ConditionTarget::Kind::EntryLink)
		{
			for (auto& l : state->child.entryLinks)
			{
				if (l.id != target.linkId) continue;

				std::string fromName;
				ImVec4 unused;
				FindTransitionSource(l.entryTransitionId, fromName, unused);
				outTitle = state->name + " / Entry(" + fromName + ") -> " + clipName(l.toClipId);
				return &l.condition;
			}
		}
		else
		{
			for (auto& l : state->child.clipLinks)
			{
				if (l.id != target.linkId) continue;

				outTitle = state->name + " / " + clipName(l.fromClipId) + " -> " + clipName(l.toClipId);
				return &l.condition;
			}
		}
		return nullptr;
	}

	default:
		return nullptr;
	}
}

// 親のリンクのIDから、遷移元の名前と色を探す
bool StudioStateGraph::FindTransitionSource(int transitionId, std::string& outName, ImVec4& outColor) const
{
	// Entryから出るリンク
	for (const auto& t : data.entry.transitions)
	{
		if (t.id != transitionId) continue;
		outName = "Entry";
		outColor = ImVec4(46.0f / 255.0f, 125.0f / 255.0f, 50.0f / 255.0f, 1.0f);
		return true;
	}

	// 各Stateから出るリンク
	for (const auto& s : data.states)
	{
		for (const auto& t : s.transitions)
		{
			if (t.id != transitionId) continue;
			outName = s.name;
			outColor = ImVec4(s.editorColor[0], s.editorColor[1], s.editorColor[2], 1.0f);
			return true;
		}
	}
	return false;
}
