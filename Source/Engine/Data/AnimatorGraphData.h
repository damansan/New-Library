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
enum class LogicOp // 論理演算子の種類
{
	And, // かつ
	Or,  // または
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
struct ConditionLogicNode // 論理演算ノード
{
	int id = -1;
	float posX = 0.0f;
	float posY = 0.0f;

	LogicOp op = LogicOp::And;
};
struct ConditionNotNode // 否定ノード
{
	int id = -1;
	float posX = 0.0f;
	float posY = 0.0f;
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
	std::vector<ConditionLogicNode>    logicNodes;
	std::vector<ConditionNotNode>      notNodes;

	std::vector<ConditionLink> links;   // Getter/Variable/Operator/Return同士を繋ぐリンク

	// ID採番カウンター
	int* idCounter = nullptr;
	int GenerateId() { return (*idCounter)++; }
};


// == Stateグラグ本体 == //
struct AnimLink // リンク
{
	int id = -1;     // このリンクのID
	int fromId = -1; // 遷移元のID
	int toId = -1;   // 遷移先のID

	bool blend = true;         // 遷移時にブレンドするか
	float blendDuration = 0.2f; // ブレンドにかける秒数

	ConditionGraph condition; // このLinkの遷移条件グラフ
};

struct ChildState  // 子階層State
{
	int id = -1;
	std::string name;

	float posX = 0.0f;
	float posY = 0.0f;

	ChildStateType type = ChildStateType::Normal;

	std::string clipName;       // 再生するアニメーション名
	bool loop = true;           // ループ再生するか
	bool useRootMotion = false; // ルートモーションの使用
};
struct ChildStateGraph // 子階層グラフ
{
	std::vector<ChildState> children;
	std::vector<AnimLink> childLinks;

	// ID採番カウンター
	int* idCounter = nullptr;
	int GenerateId() { return (*idCounter)++; }

	// 子階層のStateを作成する
	int CreateChildState(const std::string& name, float posX, float posY);
	// 子階層のLinkを作成する
	int CreateChildLink(int fromId, int toId);

	// 子階層のStateを削除する
	void DeleteChildState(int stateId);
	// 子階層のLinkを削除する
	void DeleteChildLink(int linkId);

	// Start/Endがなければ作成する(古い保存データの復元時にも使う)
	void EnsureStartAndEndExist();
};

struct ParentState  // 親階層State
{
	int id = -1;
	std::string name;

	float posX = 0.0f;
	float posY = 0.0f;

	ParentStateType type = ParentStateType::State;

	float color[3] = { 0.27f, 0.35f, 0.47f };

	ChildStateGraph childGraph; // 親階層は子階層グラフを持つ
};
struct ParentStateGraph // 親階層グラフ
{
	std::vector<ParentState> states;
	std::vector<AnimLink> stateLinks;

	// ID採番カウンター
	int* idCounter = nullptr;
	int GenerateId() { return (*idCounter)++; }

	// 親階層のStateを作成する
	int CreateParentState(const std::string& name, float posX, float posY);
	// 親階層のLinkを作成する
	int CreateParentLink(int fromId, int toId);

	// 親階層のStateを削除する
	void DeleteParentState(int stateId);
	// 親階層のLinkを削除する
	void DeleteParentLink(int linkId);

	// Entryがなければ作成する(古い保存データの復元時にも使う)
	void EnsureEntryExists();
};


struct TransitionLink // 遷移用リンク1本分
{
	int id = -1;
	int fromId = -1;
	int toId = -1;  

	ConditionGraph condition;
};
struct TransitionGraphData // 遷移用グラフ本体
{
	std::vector<TransitionLink> links;

	// ID採番カウンター
	int* idCounter = nullptr;
	int GenerateId() { return (*idCounter)++; }

	// Link作成
	int CreateLink(int fromId, int toId);
	// Link削除
	void DeleteLink(int linkId);
};



class AnimatorGraphData
{
public:
	AnimatorGraphData();

	// コピー禁止
	AnimatorGraphData(const AnimatorGraphData&) = delete;
	AnimatorGraphData& operator=(const AnimatorGraphData&) = delete;
	// ムーブ禁止
	AnimatorGraphData(AnimatorGraphData&&) = delete;
	AnimatorGraphData& operator=(AnimatorGraphData&&) = delete;

	ParentStateGraph parentGraph; // 親階層グラフ本体
	TransitionGraphData transitionGraph; // 遷移用グラフ本体

	// グラフ内容をJsonに変換する
	nlohmann::ordered_json ToJson() const;
	// Jsonから中身を変換する
	void FromJson(const nlohmann::json& j);

	// ParentStateを削除する
	void DeleteParentState(int stateId);
	void SyncTransitionLinks();
private:
	// Statesを保存内容へ変換
	nlohmann::ordered_json StatesToJson() const;
	// Statesを保存内容から復元する
	void StatesFromJson(const nlohmann::json& j);

	// StateLinksを保存内容へ変換
	nlohmann::ordered_json StateLinksToJson() const;
	// StateLinksを保存内容から復元する
	void StateLinksFromJson(const nlohmann::json& j);

	// TransitionLinksの保存内容へ変換
	nlohmann::ordered_json TransitionLinksToJson() const;
	// TransitionLinksの保存内容から復元
	void TransitionLinksFromJson(const nlohmann::json& j);

	// AnimLink1本分をJsonに変換する(親Link・子Link共通)
	nlohmann::ordered_json AnimLinkToJson(const AnimLink& link) const;
	// AnimLink1本分をJsonから復元する(親Link・子Link共通)
	void AnimLinkFromJson(AnimLink& link, const nlohmann::json& jl);

	// 読み込んだIDにカウンタを追従させる
	void SyncIdCounter(int id);

	// ID採番カウンターの実体
	int idCounter = 1;
};