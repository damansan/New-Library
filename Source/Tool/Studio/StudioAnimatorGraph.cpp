#include "StudioAnimatorGraph.h"
#include "ECS/Components/SkinnedMeshComponent.h"
#include <json.hpp>

void StudioAnimatorGraph::Initialize()
{
	parentStateGraph.Initialize();
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

	if (showConditionGraph)
	{
		DrawConditionCanvas();
	}
	else
	{
		DrawStateCanvas();
	}

	ImGui::Columns(1);

	ImGui::End();
}

// 親階層(State)キャンバス
void StudioAnimatorGraph::DrawStateCanvas()
{
	parentStateGraph.DrawGUI(graphData, [this](AnimLink* link)
		{
			conditionGraph.SetTarget(link);
			showConditionGraph = true;
		});

	ImGui::NextColumn();

	parentStateGraph.DrawInspector();
}
// 遷移条件キャンバス
void StudioAnimatorGraph::DrawConditionCanvas()
{
	if (ImGui::Button("< Back to States"))
	{
		showConditionGraph = false;
	}

	conditionGraph.DrawGUI();

	ImGui::NextColumn();

	conditionGraph.DrawInspector();
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
	if (!context.attachedGraphPath.empty())
	{
		if (ImGui::Button("Save"))
		{
			Save(context);
		}
		ImGui::SameLine();
		ImGui::TextDisabled("%s", context.attachedGraphPath.c_str());
	}
	else
	{
		ImGui::TextDisabled("No graph attached.");
	}
	ImGui::Separator();

	DrawDropTarget(context);
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
	if (context.attachedGraphPath.empty()) return;

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