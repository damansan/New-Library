#pragma once
#include <string>
#include <vector>

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

	std::vector<AnimNode> children;
	std::vector<AnimLink> childLinks;
};

class AnimatorGraph
{
public:
	std::vector<AnimState>states;
	std::vector<AnimLink>stateLinks;

	// Stateを作成する
	int CreateState(const std::string& name, float posX, float posY);
	// Stateを削除する
	void DeleteState(int stateId);

	int GenerateId() { return nextId++; }

private:
	int nextId = 1;
};