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
struct BlendSetting
{
	bool enabled = true;   // ブレンドするか
	float duration = 0.2f; // ブレンドの秒数(enabledがfalseなら使わない)
};

// 親階層 : 遷移(State → State)
struct StateTransition
{
	int id = -1;
	int toStateId = -1;
	bool always = false; // trueなら条件に関係なく成立する
	BlendSetting blend;
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

// 子階層：遷移(エントリー → クリップ)
struct EntryLink
{
	int id = -1;
	int entryTransitionId = -1; // どの遷移から来たエントリーか
	int toClipId = -1;
	bool always = false;
	ConditionGraph condition;
};

// 子階層：遷移(クリップ → クリップ)
struct ClipLink
{
	int id = -1;
	int fromClipId = -1;
	int toClipId = -1;
	bool always = false;
	BlendSetting blend;
	ConditionGraph condition;
};

// 子階層(そのStateで何を再生するか)
struct ChildGraph
{
	std::vector<ClipNode>  clips;
	std::vector<EntryLink> entryLinks;
	std::vector<ClipLink>  clipLinks;

	// エントリーのエディタ上の位置
	std::unordered_map<int, EditorPosition> entryEditorPos;

	const ClipNode* FindClip(int clipId) const;
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

	// 整合性チェック
	bool Validate(const std::string& sourceName) const;

	// JSONとの変換
	nlohmann::ordered_json ToJson() const;
	bool FromJson(const nlohmann::json& j, const std::string& sourceName);

private:
	int nextId = 1;
};