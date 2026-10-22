#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <json.hpp>
#include "Engine/Data/ConditionGraphData.h"
#include "Engine/Data/StateMachineCommon.h"
#include "Engine/Data/StateMachineBlendData.h"

// ブレンドの設定
struct CrossfadeSetting
{
	bool enabled = true;   // クロスフェードするか
	float duration = 0.2f; // クロスフェードの秒数(enabledがfalseなら使わない)
};

// 親階層：遷移(State → State)
struct StateTransition
{
	int id = -1;
	int toStateId = -1;
	bool always = false; // trueなら条件に関係なく成立する
	CrossfadeSetting crossfade;
	ConditionGraph condition;
};

// 子階層：クリップを1本再生するノード
struct ClipNode
{
	int id = -1;
	std::string name;
	std::string clipName;
	bool loop = true;
	bool rootMotion = false;
	EditorPosition editorPos;
};

// 子階層：複数のクリップを混ぜて再生するノード
struct BlendNode
{
	int id = -1;
	std::string name;
	bool loop = true;
	EditorPosition editorPos;
	BlendGraph graph; // 中のグラフ
};

// 子階層のノードの種類
enum class ChildNodeKind
{
	None,  // 見つからない
	Clip,  // ClipNode
	Blend, // BlendNode
};

// 子階層：遷移(エントリー → ノード)
struct EntryLink
{
	int id = -1;
	int entryTransitionId = -1; // どの遷移から来たエントリーか
	int toNodeId = -1;          // 行き先のノード
	bool always = false;
	ConditionGraph condition;
};

// 子階層：遷移(ノード → ノード)
struct ClipLink
{
	int id = -1;
	int fromNodeId = -1; // 遷移元のノード
	int toNodeId = -1;   // 行き先のノード
	bool always = false;
	CrossfadeSetting crossfade;
	ConditionGraph condition;
};

// 子階層
struct ChildGraph
{
	std::vector<ClipNode>  clips;
	std::vector<BlendNode> blends;
	std::vector<EntryLink> entryLinks;
	std::vector<ClipLink>  clipLinks;

	// エントリーのエディタ上の位置
	std::unordered_map<int, EditorPosition> entryEditorPos;

	// 検索
	const ClipNode* FindClip(int clipId) const;
	const BlendNode* FindBlend(int blendId) const;

	// ノードの種類
	ChildNodeKind GetNodeKind(int nodeId) const;
	// ノードの名前
	const std::string* FindNodeName(int nodeId) const;
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

	// 遷移元にEntryを指定するときのID
	static constexpr int kEntryId = 0;

	StateMachineEntry entry;
	std::vector<StateNode> states;

	// ID採番
	int GenerateId() { return nextId++; }

	// 検索
	const StateNode* FindState(int stateId) const;
	const StateNode* FindStateByName(const std::string& name) const;
	const StateTransition* FindTransition(int transitionId) const; // Entryの遷移も含む

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

	// 子階層：Blend
	int  AddBlend(int stateId, const std::string& name, const EditorPosition& pos); // 作成
	void RemoveBlend(int stateId, int blendId);                                     // 削除

	// 子階層：遷移(エントリー → ノード)
	int  AddEntryLink(int stateId, int transitionId, int toNodeId); // 作成
	void RemoveEntryLink(int stateId, int linkId);                  // 削除
	bool MoveEntryLink(int stateId, int linkId, int offset);        // 優先度の入れ替え

	// 子階層：遷移(ノード → ノード)
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

	// 遷移を持っているリストを探す
	std::vector<StateTransition>* FindTransitionOwner(int transitionId);
};