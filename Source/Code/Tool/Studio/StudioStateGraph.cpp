#include "StudioStateGraph.h"
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <imgui.h>
#include <json.hpp>
#include <cmath>
#include <cfloat>

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
	if (open)
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

		// 右：インスペクター(c-2で作る。今は選択の確認用)
		int linkId = parentCanvas.GetSelectedLinkId();
		int stateId = parentCanvas.GetSelectedNodeKey();
		if (linkId != -1)
		{
			ImGui::Text("Link: %d", linkId);
		}
		else if (stateId == StateMachineData::kEntryId)
		{
			ImGui::Text("Entry");
		}
		else if (const StateNode* s = std::as_const(data).FindState(stateId))
		{
			ImGui::Text("State: %s", s->name.c_str());
		}
		else
		{
			ImGui::TextDisabled("Nothing selected.");
		}

		// 選択している物の削除(文字を入力中は、Deleteキーを文字の削除に使うので除く)
		if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
			!ImGui::GetIO().WantTextInput &&
			ImGui::IsKeyPressed(ImGuiKey_Delete))
		{
			parentCanvas.DeleteSelection(data);
		}

		ImGui::Columns(1);
	}

	ImGui::End(); 
}

// ツールバー
void StudioStateGraph::DrawToolbar(StudioContext& context)
{
	if (ImGui::Button("New"))
	{
		NewGraph(context);
	}
	ImGui::SameLine();
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

		message.clear();
	}
	catch (const std::exception&)
	{
		message = "Invalid json.";
	}
}

// 新規作成
void StudioStateGraph::NewGraph(StudioContext& context)
{
	data.Clear();

	// 選択などを戻す
	parentCanvas.Reset();

	context.attachedGraphPath.clear();
	loaded = true;     // 空のグラフを、読み込み済みとして扱う
	hasErrors = false; // 作り始めなので、検証は保存時に行う
	message.clear();
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