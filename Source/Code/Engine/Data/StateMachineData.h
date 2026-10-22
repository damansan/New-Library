#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <json.hpp>
#include "Engine/Data/ConditionGraphData.h"

// エディタ上の位置
struct EditorPosition
{
	float x = 0.0f;
	float y = 0.0f;
};

// ブレンドの設定
struct CrossfadeSetting
{
	bool enabled = true;   // クロスフェードするか
	float duration = 0.2f; // クロスフェードの秒数(enabledがfalseなら使わない)
};

// 親階層 : 遷移(State → State)
struct StateTransition
{
	int id = -1;
	int toStateId = -1;
	bool always = false; // trueなら条件に関係なく成立する
	CrossfadeSetting crossfade;
	ConditionGraph condition;
};

// 子階層：再生するクリップ
struct ClipNode
{
	int id = -1;
	std::string name;
	std::string clipName;
	bool loop = true;
	bool rootMotion = false;
	EditorPosition editorPos;
};

// 子階層：Blend の中

// ブレンド比率をどこから取るか
enum class BlendRateSource
{
	Fixed,    // その場で決めた値
	Variable, // Component の変数から読む
};
// ブレンド の中のノードの種類
enum class BlendNodeKind
{
	None,     // 見つからない
	Output,   // 出力のノード
	Source,   // アニメーションの元ノード
	PairLink, // 2つのノードをブレンドするリンク
};
// ブレンドを行う材料(クリップを1本再生する)
struct BlendSourceNode
{
	int id = -1;
	std::string clipName;
	EditorPosition editorPos;
};
// ブレンドを行うペア(材料2つ(A・B)を結ぶ線　比率で混ぜた結果を　線の真ん中から出す)
struct BlendPairLink
{
	int id = -1;
	int fromId = -1; // A(材料のID)
	int toId = -1;   // B(材料のID)

	BlendRateSource rateSource = BlendRateSource::Fixed;

	// その場で決めた値 のときに使う
	float fixedRate = 0.5f;

	// コンポーネントの変数を取得する ときに使う
	std::string componentName;
	std::string fieldName;
	FloatComponent selectedComponent = FloatComponent::None;
};
// Blend の終着点
struct BlendOutputNode
{
	int id = -1;
	EditorPosition editorPos{ 500.0f, 200.0f };
};
// Output への線(条件を持つと優先度を持つ)
struct BlendOutputLink
{
	int id = -1;
	int fromId = -1; // 材料かペアのID
	bool always = false;
	ConditionGraph condition;
};
// Blend のグラフ
struct BlendGraph
{
	BlendOutputNode outputNode;
	std::vector<BlendSourceNode> sources;
	std::vector<BlendPairLink>   pairLinks;
	std::vector<BlendOutputLink> outputLinks; // 並び順がそのまま優先度

	// 次に割り当てるID
	int nextId = 2;

	BlendGraph() { outputNode.id = 1; }

	int GenerateId() { return nextId++; }

	// 使用中のIDの最大値+1に、採番をそろえる
	void RecalculateNextId();

	// 検索
	const BlendSourceNode* FindSource(int id) const;
	const BlendPairLink* FindPairLink(int id) const;
	const BlendOutputLink* FindOutputLink(int id) const;

	// ノードの種類
	BlendNodeKind GetNodeKind(int id) const;
};
// 複数のクリップをブレンドし再生するノード
struct BlendNode
{
	int id = -1;
	std::string name;
	bool loop = true;        
	EditorPosition editorPos;
	BlendGraph graph;         // 中のグラフ(材料・ペア・Output)
};

// 子階層の中のノードの種類
enum class ChildNodeKind
{
	None,  // 見つからない
	Clip,  // アニメーションクリップ
	Blend, // アニメーションブレンド
};

// 子階層：遷移(エントリー → ノード)
struct EntryLink
{
	int id = -1;
	int entryTransitionId = -1; // どの遷移から来たエントリーか
	int toNodeId = -1;
	bool always = false;
	ConditionGraph condition;
};

// 子階層：遷移(ノード → ノード)
struct ClipLink
{
	int id = -1;
	int fromNodeId = -1;        // 遷移元のノード
	int toNodeId = -1;          // 行き先のノード
	bool always = false;
	CrossfadeSetting crossfade;
	ConditionGraph condition;
};

// 子階層(そのStateで何を再生するか)
struct ChildGraph
{
	std::vector<ClipNode>  clips;
	std::vector<BlendNode> blends;
	std::vector<EntryLink> entryLinks;
	std::vector<ClipLink>  clipLinks;

	// エントリーのエディタ上の位置
	std::unordered_map<int, EditorPosition> entryEditorPos;

	const ClipNode* FindClip(int clipId) const;
	const BlendNode* FindBlend(int blendId) const;

	// ノードの種類
	ChildNodeKind GetNodeKind(int nodeId) const;
};

// State(親)
struct StateNode
{
	int id = -1;
	std::string name;
	std::vector<StateTransition> transitions; // 並び順がそのまま優先度
	ChildGraph child;

	EditorPosition editorPos;
	float editorColor[3] = { 0.27f, 0.35f, 0.47f };
};

// 親のEntry(最初のStateへの遷移を持つ)
struct StateMachineEntry
{
	std::vector<StateTransition> transitions;
	EditorPosition editorPos;
};

// Stateマシンのグラフ全体
class StateMachineData
{
public:
	static constexpr int kVersion = 2;

	StateMachineEntry entry;
	std::vector<StateNode> states;

	// ID採番
	int GenerateId() { return nextId++; }

	// 検索
	const StateNode* FindState(int stateId) const;
	const StateNode* FindStateByName(const std::string& name) const;
	const StateTransition* FindTransition(int transitionId) const; // Entryの遷移も含む

	// 遷移元にEntryを指定するときのID
	static constexpr int kEntryId = 0;

	
	
	// 空のグラフにする
	void Clear();

	// State
	int  AddState(const std::string& baseName, const EditorPosition& pos); // 作成
	bool RenameState(int stateId, const std::string& newName);             // 名前変更
	void RemoveState(int stateId);                                         // 削除

	// 遷移
	int  AddTransition(int fromStateId, int toStateId); // 作成
	void RemoveTransition(int transitionId);            // 削除
	bool MoveTransition(int transitionId, int offset);  // 優先度の入れ替え

	// 子階層：クリップ
	int  AddClip(int stateId, const std::string& name, const EditorPosition& pos); // 作成
	void RemoveClip(int stateId, int clipId);                                      // 削除

	// 子階層：(エントリー → クリップ)
	int  AddEntryLink(int stateId, int transitionId, int toNodeId); // 作成
	void RemoveEntryLink(int stateId, int linkId);                  // 削除
	bool MoveEntryLink(int stateId, int linkId, int offset);        // 優先度の入れ替え

	// 子階層：(クリップ → クリップ)
	int  AddClipLink(int stateId, int fromNodeId, int toNodeId); // 作成
	void RemoveClipLink(int stateId, int linkId);                // 削除
	bool MoveClipLink(int stateId, int linkId, int offset);      // 優先度の入れ替え

	// 編集用の検索
	StateNode* FindState(int stateId);
	StateTransition* FindTransition(int transitionId);

	// 他のStateと重複しない名前を作る
	std::string MakeUniqueStateName(const std::string& baseName) const;



	// 整合性チェック
	bool Validate(const std::string& sourceName) const;

	// JSONとの変換
	nlohmann::ordered_json ToJson() const;
	bool FromJson(const nlohmann::json& j, const std::string& sourceName);

private:
	int nextId = 1;

	// 遷移を持っているリスト(Entry、またはどれかのState)を探す
	std::vector<StateTransition>* FindTransitionOwner(int transitionId);
};