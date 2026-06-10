#include "ProjectWindow.h"
#include <imgui.h>

#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/MeshComponent.h"
#include "ECS/Components/MaterialComponent.h"
#include <algorithm>
#include <WICTextureLoader.h>

void ProjectWindow::Initialize(const std::string& rootPath)
{
	this->rootPath = rootPath;
	// 初期選択フォルダはルート
	this->selectedFolder = rootPath;
}

// アイコン読み込み
void ProjectWindow::LoadIcons(ID3D11Device* device)
{
	// Data/Sprite/System/folder.png を読み込む
	DirectX::CreateWICTextureFromFile(
		device,
		L"Data/Sprite/System/folder.png",
		nullptr,
		modelIconSRV.GetAddressOf()
	);
}

void ProjectWindow::Draw(entt::registry& registry, EditorContext& context)
{
	ImGui::Begin("Project");

	// 左右に分割
	ImGui::Columns(2, "ProjectColumns", true);
	// 左側の幅を固定
	ImGui::SetColumnWidth(0, 200.0f);

	// 左側：フォルダツリー
	ImGui::BeginChild("FolderTree", ImVec2(0, 0), false);
	DrawFolderTree(rootPath);
	ImGui::EndChild();

	ImGui::NextColumn();

	// 右側：ファイルグリッド

	ImGui::BeginChild("FileGrid", ImVec2(0, 0), false);
	DrawFileGrid(registry, context);
	ImGui::EndChild();

	ImGui::Columns(1);

	ImGui::End();
}

// フォルダツリー
void ProjectWindow::DrawFolderTree(const std::filesystem::path& path)
{
	if (!std::filesystem::is_directory(path)) return;

	std::string folderName = path.filename().string();

	// サブフォルダがあるか確認
	bool hasSubFolder = false;
	for (const auto& entry : std::filesystem::directory_iterator(path))
	{
		if (entry.is_directory())
		{
			hasSubFolder = true;
			break;
		}
	}

	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth;

	// 選択中フォルダをハイライト
	if (selectedFolder == path)
	{
		flags |= ImGuiTreeNodeFlags_Selected;
	}

	// 最下層の場合は葉ノードとして表示
	if (!hasSubFolder)
	{
		flags |= ImGuiTreeNodeFlags_Leaf;
		flags |= ImGuiTreeNodeFlags_NoTreePushOnOpen;
	}

	bool open = ImGui::TreeNodeEx(folderName.c_str(), flags);

	// クリックで選択
	if (ImGui::IsItemClicked())
	{
		selectedFolder = path;
	}

	// 最下層はTreePopが不要
	if (open && hasSubFolder)
	{
		for (const auto& entry : std::filesystem::directory_iterator(path))
		{
			if (entry.is_directory())
			{
				DrawFolderTree(entry.path());
			}
		}
		ImGui::TreePop();
	}
}
// ファイル一覧
void ProjectWindow::DrawFileGrid(entt::registry& registry, EditorContext& context)
{
	if (selectedFolder.empty()) return;
	if (!std::filesystem::exists(selectedFolder)) return;

	// アイコンサイズと余白
	const float iconSize = 64.0f;
	const float padding = 16.0f;
	const float cellSize = iconSize + padding;

	// ウィンドウ幅から列数を計算
	float windowWidth = ImGui::GetContentRegionAvail().x;
	int   columns = (std::max)(1, (int)(windowWidth / cellSize));

	int index = 0;

	for (const auto& entry : std::filesystem::directory_iterator(selectedFolder))
	{
		if (!entry.is_regular_file()) continue;
		if (!IsModelFile(entry.path())) continue;

		std::string filename = entry.path().stem().string();
		std::string modelPath = entry.path().string();
		std::replace(modelPath.begin(), modelPath.end(), '\\', '/');

		// 列の折り返し
		if (index % columns != 0)
		{
			ImGui::SameLine();
		}

		// グループ化してアイコン＋名前をまとめる
		ImGui::BeginGroup();

		ImVec2 pos = ImGui::GetCursorScreenPos();

		// アイコン表示
		if (modelIconSRV)
		{
			ImGui::Image(
				(ImTextureID)modelIconSRV.Get(),
				ImVec2(iconSize, iconSize)
			);
		}
		else
		{
			ImGui::GetWindowDrawList()->AddRectFilled(
				pos,
				ImVec2(pos.x + iconSize, pos.y + iconSize),
				IM_COL32(80, 120, 200, 255),
				8.0f
			);
			ImGui::Dummy(ImVec2(iconSize, iconSize));
		}

		// 画像の上に透明ボタンを重ねてドラッグ判定
		ImGui::SetCursorScreenPos(pos);
		ImGui::PushID(index);
		ImGui::InvisibleButton("##icon", ImVec2(iconSize, iconSize));

		// ドラッグ開始
		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
		{
			// モデルパスをペイロードとして渡す
			ImGui::SetDragDropPayload(
				"MODEL_PATH",
				modelPath.c_str(),
				modelPath.size() + 1
			);
			if (modelIconSRV)
			{
				ImGui::Image(
					(ImTextureID)modelIconSRV.Get(),
					ImVec2(iconSize, iconSize)
				);
			}
			ImGui::Text(filename.c_str());
			ImGui::EndDragDropSource();
		}

		ImGui::PopID();

		// ファイル名（長ければ省略）
		std::string displayName = filename;
		if (displayName.size() > 10)
			displayName = displayName.substr(0, 9) + "...";
		ImGui::TextUnformatted(displayName.c_str());

		ImGui::EndGroup();
		index++;
	}
}


bool ProjectWindow::IsModelFile(const std::filesystem::path& path)
{
	std::string ext = path.extension().string();

	// 小文字に変換して比較
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

	return ext == ".gltf" || ext == ".glb";
}