#pragma once
#include <string>
#include <vector>
#include <json.hpp>

// 子階層ノードの種類
enum class AnimNodeType
{
	Normal, // アニメーションノード
	Start,  // 開始点
	End,    // 終了点
};

// 親階層のノードの種類
enum class AnimStateType
{
	Entry, // 開始点
	State, // ノード
};

// リンク
struct AnimLink
{
	int id = -1;     // このリンクのID
	int fromId = -1; // 遷移元のID
	int toId = -1;   // 遷移先のID
};

// 子階層ノード
struct AnimNode
{
	int id = -1;
	std::string name;

	float posX = 0.0f;
	float posY = 0.0f;

	AnimNodeType type = AnimNodeType::Normal;
	std::string clipName;
};

// 親階層ノード
struct AnimState
{
	int id = -1;
	std::string name;

	float posX = 0.0f;
	float posY = 0.0f;

	AnimStateType type = AnimStateType::State;

	float color[3] = { 0.27f, 0.35f, 0.47f };

	std::vector<AnimNode> children;
	std::vector<AnimLink> childLinks;
};

class AnimatorGraph
{
public:
	AnimatorGraph();

	std::vector<AnimState>states;
	std::vector<AnimLink>stateLinks;

	// Stateを作成する
	int CreateState(const std::string& name, float posX, float posY);
	// Stateを削除する
	void DeleteState(int stateId);

	// Linkを作成する
	int CreateLink(int fromId, int toId);

	int GenerateId() { return nextId++; }

	// グラフ内容をJsonに変換する
	nlohmann::ordered_json ToJson() const;
	// Jsonから中身を変換する
	void FromJson(const nlohmann::json& j);

private:
	// Entryがなければ作成する
	void EnsureEntryExists();

	int nextId = 1;
};