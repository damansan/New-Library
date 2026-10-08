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
}

void StudioStateGraph::DrawGUI(entt::registry& registry, StudioContext& context)
{
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
	}

	ImGui::End(); 

	// 開いている子のウィンドウを描く
	DrawChildWindows();
}

// モデルが変わったときに呼ぶ
void StudioStateGraph::OnModelChanged(StudioContext& context)
{
	data.Clear();

	// 選択などを戻す
	parentCanvas.Reset();
	childCanvases.clear();

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
			child.Draw(data);
		}
		ImGui::End();

		// ×で閉じられたら、一覧から外す
		if (!open) it = childCanvases.erase(it);
		else ++it;
	}
}