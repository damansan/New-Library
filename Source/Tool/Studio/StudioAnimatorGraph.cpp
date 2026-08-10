#include "StudioAnimatorGraph.h"
#include "ECS/Components/SkinnedMeshComponent.h"
#include "Editor/ProjectWindow.h"
#include "Tool/Studio/AnimatorGraphNavigation.h"
#include <json.hpp>
#include <filesystem>
#include <algorithm>

void StudioAnimatorGraph::Initialize()
{
	parentStateGraph.Initialize();
	childStateGraph.Initialize();
	conditionGraph.Initialize();
}

// ImGui描画
void StudioAnimatorGraph::DrawGUI(entt::registry& registry, StudioContext& context)
{
	if (!context.showAnimatorGraph) return;

	if (!loaded && !context.attachedGraphPath.empty())
	{
		Load(context);
		loaded = true;
		parentStateGraph.RequestApplyPositions();
	}

	RefreshAvailableAnimations(registry);

	std::string panelName = "AnimatorGraph##" + std::to_string(windowId);
	ImGui::Begin(panelName.c_str());

	DrawToolbar(context);

	ImGui::Columns(2, "AnimatorGraphColumns", true);

	if (!columnWidthInitialized)
	{
		ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * (3.0f / 4.0f));
		columnWidthInitialized = true;
	}

	if (ImGui::GetColumnWidth(0) > ImGui::GetWindowWidth() - 200.0f)
	{
		ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() - 220.0f);
	}

	switch (currentView)
	{
	case AnimatorGraphView::ChildGraph:
		DrawChildCanvas();
		break;
	case AnimatorGraphView::ConditionGraph:
		DrawConditionCanvas();
		break;
	case AnimatorGraphView::ParentGraph:
	default:
		DrawParentCanvas(context);
		break;
	}

	ImGui::Columns(1);

	ImGui::End();
}

// 親階層(State)キャンバス
void StudioAnimatorGraph::DrawParentCanvas(StudioContext& context)
{
	parentStateGraph.DrawGUI(graphData);

	// ドロップ対象として紐づける
	DrawDropTarget(context);

	ImGui::NextColumn();

	parentStateGraph.DrawInspector();

	// 画面遷移の要求を確認する
	AnimLink* navLink = nullptr;
	ParentState* navState = nullptr;
	NavigationRequest request = parentStateGraph.ConsumeNavigationRequest(navLink, navState);

	if (request == NavigationRequest::EditCondition && navLink)
	{
		conditionGraph.SetTarget(navLink);
		GoToCondition();
	}
	else if (request == NavigationRequest::EditChildGraph && navState)
	{
		GoToChildGraph(navState);
	}
}
// 子階層キャンバス
void StudioAnimatorGraph::DrawChildCanvas()
{
	if (ImGui::Button("< Back to Parent"))
	{
		currentView = AnimatorGraphView::ParentGraph;
		editingParent = nullptr;
	}

	if (!editingParent)
	{
		// 対象を見失った場合(戻るボタン押下直後含む)、親画面へ
		currentView = AnimatorGraphView::ParentGraph;
		ImGui::NextColumn();
		return;
	}

	ImGui::SameLine();
	ImGui::TextDisabled("Editing: %s", editingParent->name.c_str());

	childStateGraph.DrawGUI(editingParent->childGraph, availableAnimations);

	ImGui::NextColumn();

	childStateGraph.DrawInspector();

	AnimLink* navLink = nullptr;
	NavigationRequest request = childStateGraph.ConsumeNavigationRequest(navLink);

	if (request == NavigationRequest::EditCondition && navLink)
	{
		conditionGraph.SetTarget(navLink);
		GoToCondition();
	}
}
// 遷移条件キャンバス
void StudioAnimatorGraph::DrawConditionCanvas()
{
	if (ImGui::Button("< Back"))
	{
		currentView = previousView;
	}

	conditionGraph.DrawGUI();

	ImGui::NextColumn();

	conditionGraph.DrawInspector();
}

// 条件グラフへ移動する
void StudioAnimatorGraph::GoToCondition()
{
	previousView = currentView;
	currentView = AnimatorGraphView::ConditionGraph;
}
// 子階層グラフへ移動する
void StudioAnimatorGraph::GoToChildGraph(ParentState* state)
{
	editingParent = state;
	currentView = AnimatorGraphView::ChildGraph;

	childStateGraph.RequestApplyPositions();
}

// 上側ツールバー
void StudioAnimatorGraph::DrawToolbar(StudioContext& context)
{
	// エラーメッセージ
	if (!attachError.empty())
	{
		ImGui::TextColored(ImVec4(1, 0.4f, 0.4f, 1), "%s", attachError.c_str());
	}

	// 保存ボタン
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
	ImGui::Separator();
}

// .animgraphのD&D受け口
void StudioAnimatorGraph::DrawDropTarget(StudioContext& context)
{
	// パネル全体を.animgraphのドロップ対象にする
	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ANIMGRAPH_PATH"))
		{
			std::string droppedPath(static_cast<const char*>(payload->Data));
			TryAttachGraph(droppedPath, context);
		}
		ImGui::EndDragDropTarget();
	}
}
// ドロップされたパスを紐づけ可能か検証する
void StudioAnimatorGraph::TryAttachGraph(const std::string& path, StudioContext& context)
{
	std::ifstream ifs(path);
	if (!ifs.is_open())
	{
		attachError = "Failed to open file.";
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
		attachError = "Invalid json.";
		return;
	}

	// 新規作成Jsonは受け入れ、既存Jsonならモデルパスを比較
	if (!sourceModelPath.empty() && sourceModelPath != context.currentModelPath)
	{
		attachError = "This graph belongs to a different model.";
		return;
	}

	attachError.clear();
	context.attachedGraphPath = path;
	loaded = false;
}

void StudioAnimatorGraph::RefreshAvailableAnimations(entt::registry& registry)
{
	availableAnimations.clear();

	auto view = registry.view<SkinnedMeshComponent>();
	if (view.begin() == view.end()) return;

	auto entity = *view.begin();
	auto& skinned = view.get<SkinnedMeshComponent>(entity);

	if (!skinned.model) return;

	for (const auto& anim : skinned.model->GetAnimations())
	{
		availableAnimations.push_back(anim.name);
	}
}

// 保存
void StudioAnimatorGraph::Save(StudioContext& context)
{
	// 未紐づけなら、保存先を自動で決定する
	if (context.attachedGraphPath.empty())
	{
		context.attachedGraphPath = BuildNewSavePath(context);
		loaded = true;
	}

	nlohmann::ordered_json j;
	j["sourceModelPath"] = context.currentModelPath;

	nlohmann::ordered_json graphJson = graphData.ToJson();
	j["states"] = graphJson["states"];
	j["stateLinks"] = graphJson["stateLinks"];

	std::ofstream ofs(context.attachedGraphPath);
	if (ofs.is_open())
	{
		ofs << j.dump(4);
	}
}
// 読み込み
void StudioAnimatorGraph::Load(StudioContext& context)
{
	std::ifstream ifs(context.attachedGraphPath);
	if (!ifs.is_open()) return;

	try
	{
		nlohmann::json j;
		ifs >> j;
		graphData.FromJson(j);
	}
	catch (const std::exception&)
	{
		// 壊れているか空のjsonの場合なにもしない
	}
}

// 新規保存先のパスを決定する
std::string StudioAnimatorGraph::BuildNewSavePath(StudioContext& context)
{
	std::filesystem::path saveDir = "Data/Json/Animgraph";
	std::filesystem::create_directories(saveDir); // フォルダが無ければ作る

	std::string modelName = std::filesystem::path(context.currentModelPath).stem().string();
	std::string baseName = modelName + ".animgraph";

	std::string uniqueName = ProjectWindow::GenerateUniqueName(saveDir, baseName);

	std::string result = (saveDir / uniqueName).string();
	std::replace(result.begin(), result.end(), '\\', '/');
	return result;
}
