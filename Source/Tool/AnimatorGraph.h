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

// 比較演算子の種類
enum class CompareOp
{
	Greater,      // >
	Less,         // 
	Equal,        // ==
	NotEqual,     // !=
	GreaterEqual, // >=
	LessEqual,    // <=
};

// 変数ノード
struct ConditionVariableNode
{
	int id = -1;
	float posX = 0.0f;
	float posY = 0.0f;

	std::string componentName; 
	std::string fieldName; 
	float constValue = 0.0f;
};

// 比較ノード
struct ConditionOperatorNode
{
	int id = -1;
	float posX = 0.0f;
	float posY = 0.0f;

	CompareOp op = CompareOp::Greater;
	float compareValue = 0.0f;
};

// 条件グラフ内のリンク
struct ConditionLink
{
	int id = -1;
	int fromId = -1;
	int toId = -1;
};

// 条件グラフの終着点
struct ConditionReturnNode
{
	int id = -1;
	float posX = 0.0f;
	float posY = 0.0f;
};

// Link1本分の条件グラフ
struct ConditionGraph
{
	ConditionReturnNode returnNode;

	std::vector<ConditionVariableNode> variableNodes;
	std::vector<ConditionOperatorNode> operatorNodes;
	std::vector<ConditionLink> links;

	int nextId = 1;
	int GenerateId() { return nextId++; }
};

// リンク
struct AnimLink
{
	int id = -1;     // このリンクのID
	int fromId = -1; // 遷移元のID
	int toId = -1;   // 遷移先のID

	ConditionGraph condition; // このLinkの遷移条件グラフ
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

	//float color[3] = { 0.45f, 0.45f, 0.45f }; // 初期値はグレー
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
	// Linkを削除する
	void DeleteLink(int linkId);

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