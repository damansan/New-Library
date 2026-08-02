#pragma once
#include <string>
#include <vector>
#include <json.hpp>

// == Stateグラフ関連 == //
enum class ChildStateType // 子階層(ChildState)の種類
{
	Normal, // アニメーションノード
	Start,  // 開始点
	End,    // 終了点
};
enum class ParentStateType // 親階層(ParentState)の種類
{
	Entry, // 開始点
	State, // ノード
};


// == 条件グラフ関連 == //
enum class ValueType // 変数ノードが持つ値の型
{
	Int,
	Float,
	Bool,

	Float2,
	Float3,
	Float4,
};
enum class FloatComponent // 構造体のどの成分を取り出すか
{
	None,
	X, Y, Z, W,
};
enum class CompareOp // 比較演算子の種類
{
	Greater,      // >
	Less,         // <
	Equal,        // ==
	NotEqual,     // !=
	GreaterEqual, // >=
	LessEqual,    // <=
};
struct ConditionGetterNode // コンポーネントの値を取得するノード
{
	int id = -1;
	float posX = 0.0f;
	float posY = 0.0f;
	std::string componentName; // MetaRegistryで登録した型名
	std::string fieldName;     // entt::meta上のフィールド名
	ValueType type = ValueType::Float;
	FloatComponent selectedComponent = FloatComponent::None;
};  
struct ConditionVariableNode // 変数ノード
{
	int id = -1;
	float posX = 0.0f;
	float posY = 0.0f;
	std::string name; // 空の間は"Variable"と表示
	ValueType type = ValueType::Float;
	
	// 変数型に応じて以下のどれかを使用する
	int intValue = 0;
	float floatValue = 0.0f;
	bool boolValue = false;
};
struct ConditionOperatorNode // 比較ノード
{
	int id = -1;
	float posX = 0.0f;
	float posY = 0.0f;

	CompareOp op = CompareOp::Greater;
};
struct ConditionReturnNode // 条件グラフの終着点
{
	int id = -1;
	float posX = 0.0f;
	float posY = 0.0f;
};
struct ConditionLink // 条件グラフ内のリンク
{
	int id = -1;
	int fromId = -1;
	int toId = -1;
	bool toPinB = false;
};
struct ConditionGraph // Link1本分の条件グラフ
{
	ConditionReturnNode returnNode;

	std::vector<ConditionVariableNode> variableNodes;
	std::vector<ConditionGetterNode>   getterNodes;
	std::vector<ConditionOperatorNode> operatorNodes;
	std::vector<ConditionLink> links;   // Getter/Variable/Operator/Return同士を繋ぐリンク

	// このLink専用のID採番カウンター
	int nextId = 1;
	int GenerateId() { return nextId++; }
};


// == Stateグラグ本体 == //
struct AnimLink // リンク
{
	int id = -1;     // このリンクのID
	int fromId = -1; // 遷移元のID
	int toId = -1;   // 遷移先のID

	ConditionGraph condition; // このLinkの遷移条件グラフ
};
struct ChildState  // 子階層State
{
	int id = -1;
	std::string name;

	float posX = 0.0f;
	float posY = 0.0f;

	ChildStateType type = ChildStateType::Normal;
	std::string clipName;
};
struct ParentState  // 親階層State
{
	int id = -1;
	std::string name;

	float posX = 0.0f;
	float posY = 0.0f;

	ParentStateType type = ParentStateType::State;

	float color[3] = { 0.27f, 0.35f, 0.47f };

	std::vector<ChildState> children;
	std::vector<AnimLink> childLinks;
};


class AnimatorGraph
{
public:
	AnimatorGraph();

	std::vector<ParentState>states;
	std::vector<AnimLink>stateLinks;

	// Stateを作成する
	int CreateState(const std::string& name, float posX, float posY);
	// Stateを削除する
	void DeleteState(int stateId);

	// Linkを作成する
	int CreateLink(int fromId, int toId);
	// Linkを削除する
	void DeleteLink(int linkId);

	// State/StateLink用のID採番カウンター
	int GenerateId() { return nextId++; }

	// グラフ内容をJsonに変換する
	nlohmann::ordered_json ToJson() const;
	// Jsonから中身を変換する
	void FromJson(const nlohmann::json& j);

private:
	// Entryがなければ作成する
	void EnsureEntryExists();

	// Statesを保存内容へ変換
	nlohmann::ordered_json StatesToJson() const;
	// Statesを保存内容から復元する
	void StatesFromJson(const nlohmann::json& j);

	// StateLinksを保存内容へ変換
	nlohmann::ordered_json StateLinksToJson() const;
	// StateLinksを保存内容から復元する
	void StateLinksFromJson(const nlohmann::json& j);

	int nextId = 1;
};