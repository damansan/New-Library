#include "StateMachineData.h"
#include <algorithm>
#include <unordered_set>
#include <utility>

#include "Engine/Core/Log.h"

using namespace StateMachineCommon;

namespace
{
	// 遷移のクロスフェードの設定
	void WriteCrossfade(nlohmann::ordered_json& j, const CrossfadeSetting& crossfade)
	{
		j["blend"] = crossfade.enabled;
		j["blendDuration"] = crossfade.duration;
	}
	void ReadCrossfade(const nlohmann::json& j, CrossfadeSetting& crossfade)
	{
		crossfade.enabled = j.value("blend", true);
		crossfade.duration = j.value("blendDuration", 0.2f);
	}

	// 遷移
	nlohmann::ordered_json ToJson(const StateTransition& t)
	{
		nlohmann::ordered_json j;
		j["id"] = t.id;
		j["to"] = t.toStateId;
		WriteCondition(j, t.always, t.condition);
		WriteCrossfade(j, t.crossfade);
		return j;
	}
	StateTransition TransitionFromJson(const nlohmann::json& j)
	{
		StateTransition t;
		t.id = j.value("id", -1);
		t.toStateId = j.value("to", -1);
		ReadCondition(j, t.always, t.condition);
		ReadCrossfade(j, t.crossfade);
		return t;
	}


	// ============================================================
	// 編集
	// ============================================================

	// 同じグループの中で隣のものと入れ替える
	template<typename T, typename GroupFn>
	bool MoveInGroup(std::vector<T>& list, int linkId, int offset, GroupFn groupOf)
	{
		int index = -1;
		for (int i = 0; i < static_cast<int>(list.size()); ++i)
		{
			if (list[i].id == linkId) index = i;
		}
		if (index < 0 || offset == 0) return false;

		// offsetの向きに、同じグループの要素を探して入れ替える
		int group = groupOf(list[index]);
		int step = (offset < 0) ? -1 : 1;
		for (int i = index + step; i >= 0 && i < static_cast<int>(list.size()); i += step)
		{
			if (groupOf(list[i]) == group)
			{
				std::swap(list[index], list[i]);
				return true;
			}
		}
		return false; // 端なので動かせない
	}

	// 子階層：ノード(Clip か Blend)につながっている遷移を消す
	void RemoveLinksOfNode(ChildGraph& child, int nodeId)
	{
		auto& entryLinks = child.entryLinks;
		entryLinks.erase(
			std::remove_if(entryLinks.begin(), entryLinks.end(),
				[nodeId](const EntryLink& l) { return l.toNodeId == nodeId; }),
			entryLinks.end());

		auto& clipLinks = child.clipLinks;
		clipLinks.erase(
			std::remove_if(clipLinks.begin(), clipLinks.end(),
				[nodeId](const ClipLink& l) { return l.fromNodeId == nodeId || l.toNodeId == nodeId; }),
			clipLinks.end());
	}
}

// 検索 
const ClipNode* ChildGraph::FindClip(int clipId) const
{
	for (const auto& c : clips)
	{
		if (c.id == clipId) return &c;
	}
	return nullptr;
}
const BlendNode* ChildGraph::FindBlend(int blendId) const
{
	return FindById(blends, blendId);
}
// ノードの種類
ChildNodeKind ChildGraph::GetNodeKind(int nodeId) const
{
	if (FindClip(nodeId))  return ChildNodeKind::Clip;
	if (FindBlend(nodeId)) return ChildNodeKind::Blend;
	return ChildNodeKind::None;
}

const StateNode* StateMachineData::FindState(int stateId) const
{
	for (const auto& s : states)
	{
		if (s.id == stateId) return &s;
	}
	return nullptr;
}

const StateNode* StateMachineData::FindStateByName(const std::string& name) const
{
	for (const auto& s : states)
	{
		if (s.name == name) return &s;
	}
	return nullptr;
}

const StateTransition* StateMachineData::FindTransition(int transitionId) const
{
	for (const auto& t : entry.transitions)
	{
		if (t.id == transitionId) return &t;
	}
	for (const auto& s : states)
	{
		for (const auto& t : s.transitions)
		{
			if (t.id == transitionId) return &t;
		}
	}
	return nullptr;
}



// 空のグラフにする
void StateMachineData::Clear()
{
	entry = StateMachineEntry{};
	entry.editorPos = { 50.0f, 200.0f };
	states.clear();
	nextId = 1;
}


// State
int StateMachineData::AddState(const std::string& baseName, const EditorPosition& pos)
{
	StateNode s;
	s.id = GenerateId();
	s.name = MakeUniqueStateName(baseName);
	s.editorPos = pos;
	states.push_back(s);
	return s.id;
}
bool StateMachineData::RenameState(int stateId, const std::string& newName)
{
	StateNode* s = FindState(stateId);
	if (!s || newName.empty()) return false;
	if (s->name == newName) return true;

	// 他のStateと重複する名前は付けられない
	if (FindStateByName(newName)) return false;

	s->name = newName;
	return true;
}
void StateMachineData::RemoveState(int stateId)
{
	// 消える遷移：このStateから出る遷移と、このStateに入ってくる遷移
	std::vector<int> removedTransitions;
	for (const auto& t : entry.transitions)
	{
		if (t.toStateId == stateId) removedTransitions.push_back(t.id);
	}
	for (const auto& s : states)
	{
		for (const auto& t : s.transitions)
		{
			if (s.id == stateId || t.toStateId == stateId) removedTransitions.push_back(t.id);
		}
	}

	// 遷移の削除(行き先のエントリーも一緒に消える)
	for (int transitionId : removedTransitions)
	{
		RemoveTransition(transitionId);
	}

	// State本体の削除
	states.erase(
		std::remove_if(states.begin(), states.end(),
			[stateId](const StateNode& s) { return s.id == stateId; }),
		states.end());
}

// 遷移
int StateMachineData::AddTransition(int fromStateId, int toStateId)
{
	if (!FindState(toStateId)) return -1;

	std::vector<StateTransition>* owner = nullptr;
	if (fromStateId == kEntryId)
	{
		owner = &entry.transitions;
	}
	else if (StateNode* from = FindState(fromStateId))
	{
		owner = &from->transitions;
	}
	if (!owner) return -1;

	StateTransition t;
	t.id = GenerateId();
	t.toStateId = toStateId;
	owner->push_back(t); // 最後に追加(優先度は一番下)

	// 行き先のStateには、この遷移のエントリーが自動で存在する
	return t.id;
}
void StateMachineData::RemoveTransition(int transitionId)
{
	std::vector<StateTransition>* owner = FindTransitionOwner(transitionId);
	if (!owner) return;

	// 行き先のStateから、この遷移のエントリーに関するものを消す
	int toStateId = -1;
	for (const auto& t : *owner)
	{
		if (t.id == transitionId) toStateId = t.toStateId;
	}
	if (StateNode* to = FindState(toStateId))
	{
		auto& links = to->child.entryLinks;
		links.erase(
			std::remove_if(links.begin(), links.end(),
				[transitionId](const EntryLink& l) { return l.entryTransitionId == transitionId; }),
			links.end());
		to->child.entryEditorPos.erase(transitionId);
	}

	// 遷移本体の削除
	owner->erase(
		std::remove_if(owner->begin(), owner->end(),
			[transitionId](const StateTransition& t) { return t.id == transitionId; }),
		owner->end());
}
bool StateMachineData::MoveTransition(int transitionId, int offset)
{
	std::vector<StateTransition>* owner = FindTransitionOwner(transitionId);
	if (!owner) return false;

	int index = -1;
	for (int i = 0; i < static_cast<int>(owner->size()); ++i)
	{
		if ((*owner)[i].id == transitionId) index = i;
	}

	int newIndex = index + offset;
	if (index < 0 || newIndex < 0 || newIndex >= static_cast<int>(owner->size())) return false;

	std::swap((*owner)[index], (*owner)[newIndex]);
	return true;
}

// 子階層：クリップ
int StateMachineData::AddClip(int stateId, const std::string& name, const EditorPosition& pos)
{
	StateNode* s = FindState(stateId);
	if (!s) return -1;

	ClipNode c;
	c.id = GenerateId();
	c.name = name;
	c.editorPos = pos;
	s->child.clips.push_back(c);
	return c.id;
}
void StateMachineData::RemoveClip(int stateId, int clipId)
{
	StateNode* s = FindState(stateId);
	if (!s) return;

	// このクリップにつながっていたものを消す
	RemoveLinksOfNode(s->child, clipId);

	// クリップ本体の削除
	auto& clips = s->child.clips;
	clips.erase(
		std::remove_if(clips.begin(), clips.end(),
			[clipId](const ClipNode& c) { return c.id == clipId; }),
		clips.end());
}

// 子階層：ブレンド
int StateMachineData::AddBlend(int stateId, const std::string& name, const EditorPosition& pos)
{
	StateNode* s = FindState(stateId);
	if (!s) return -1;

	BlendNode b;
	b.id = GenerateId();
	b.name = name;
	b.editorPos = pos;
	s->child.blends.push_back(b);
	return b.id;
}
void StateMachineData::RemoveBlend(int stateId, int blendId)
{
	StateNode* s = FindState(stateId);
	if (!s) return;

	// この Blend につながっていたものを消す
	RemoveLinksOfNode(s->child, blendId);

	// Blend 本体の削除
	auto& blends = s->child.blends;
	blends.erase(
		std::remove_if(blends.begin(), blends.end(),
			[blendId](const BlendNode& b) { return b.id == blendId; }),
		blends.end());
}

// 子階層：(エントリー → ノード)
int StateMachineData::AddEntryLink(int stateId, int transitionId, int toNodeId)
{
	StateNode* s = FindState(stateId);
	if (!s || s->child.GetNodeKind(toNodeId) == ChildNodeKind::None) return -1; // 行き先は Clip か Blend

	// このStateに入ってくる遷移でなければ、エントリーは存在しない
	const StateTransition* t = FindTransition(transitionId);
	if (!t || t->toStateId != stateId) return -1;

	EntryLink l;
	l.id = GenerateId();
	l.entryTransitionId = transitionId;
	l.toNodeId = toNodeId;
	s->child.entryLinks.push_back(l);
	return l.id;
}
void StateMachineData::RemoveEntryLink(int stateId, int linkId)
{
	StateNode* s = FindState(stateId);
	if (!s) return;

	auto& links = s->child.entryLinks;
	links.erase(
		std::remove_if(links.begin(), links.end(),
			[linkId](const EntryLink& l) { return l.id == linkId; }),
		links.end());
}

// 優先度の入れ替え
bool StateMachineData::MoveEntryLink(int stateId, int linkId, int offset)
{
	StateNode* s = FindState(stateId);
	if (!s) return false;

	return MoveInGroup(s->child.entryLinks, linkId, offset,
		[](const EntryLink& l) { return l.entryTransitionId; });
}

// 子階層：(ノード → ノード)
int StateMachineData::AddClipLink(int stateId, int fromNodeId, int toNodeId)
{
	StateNode* s = FindState(stateId);
	if (!s) return -1;
	if (fromNodeId == toNodeId) return -1; // 自分自身へのつながりは作らない
	
	if (s->child.GetNodeKind(fromNodeId) == ChildNodeKind::None ||
		s->child.GetNodeKind(toNodeId) == ChildNodeKind::None) return -1; // 遷移元・行き先は Clip か Blend

	ClipLink l;
	l.id = GenerateId();
	l.fromNodeId = fromNodeId;
	l.toNodeId = toNodeId;
	s->child.clipLinks.push_back(l);
	return l.id;
}
void StateMachineData::RemoveClipLink(int stateId, int linkId)
{
	StateNode* s = FindState(stateId);
	if (!s) return;

	auto& links = s->child.clipLinks;
	links.erase(
		std::remove_if(links.begin(), links.end(),
			[linkId](const ClipLink& l) { return l.id == linkId; }),
		links.end());
}

// 優先度の入れ替え
bool StateMachineData::MoveClipLink(int stateId, int linkId, int offset)
{
	StateNode* s = FindState(stateId);
	if (!s) return false;

	return MoveInGroup(s->child.clipLinks, linkId, offset,
		[](const ClipLink& l) { return l.fromNodeId; });
}



// 整合性チェック
bool StateMachineData::Validate(const std::string& sourceName) const
{
	int errorCount = 0;

	auto error = [&](const std::string& message)
		{
			Log(u8"[StateMachine] エラー(%s): %s", sourceName.c_str(), message.c_str());
			++errorCount;
		};
	auto warning = [&](const std::string& message)
		{
			Log(u8"[StateMachine] 警告(%s): %s", sourceName.c_str(), message.c_str());
		};

	// IDの重複
	std::unordered_set<int> usedIds;
	auto checkId = [&](int id, const std::string& what)
		{
			if (id < 0)
			{
				error(what + u8" のIDが設定されていません");
			}
			else if (!usedIds.insert(id).second)
			{
				error(what + u8" のID " + std::to_string(id) + u8" が重複しています");
			}
		};

	for (const auto& t : entry.transitions) checkId(t.id, u8"Entryの遷移");
	for (const auto& s : states)
	{
		checkId(s.id, u8"State「" + s.name + u8"」");
		for (const auto& t : s.transitions)        checkId(t.id, u8"「" + s.name + u8"」の遷移");
		for (const auto& c : s.child.clips)        checkId(c.id, u8"「" + s.name + u8"」のクリップ");
		for (const auto& b : s.child.blends)       checkId(b.id, u8"「" + s.name + u8"」のBlend「" + b.name + u8"」");
		for (const auto& l : s.child.entryLinks)   checkId(l.id, u8"「" + s.name + u8"」のエントリーのつながり");
		for (const auto& l : s.child.clipLinks)    checkId(l.id, u8"「" + s.name + u8"」のクリップのつながり");
	}

	// Stateの名前(実行側は名前で参照するので、空と重複を禁止)
	std::unordered_set<std::string> usedNames;
	for (const auto& s : states)
	{
		if (s.name.empty())
		{
			error(u8"名前の無いStateがあります(ID " + std::to_string(s.id) + u8")");
		}
		else if (!usedNames.insert(s.name).second)
		{
			error(u8"State名「" + s.name + u8"」が重複しています");
		}
	}

	// Entry
	if (entry.transitions.empty())
	{
		error(u8"Entryから出る遷移がありません");
	}

	// 遷移・つながりの共通チェック
	auto checkCommon = [&](bool always, const ConditionGraph& condition, const std::string& what)
		{
			if (!always && condition.IsEmpty())
			{
				error(what + u8" の条件が空です(常に成立させる場合はalwaysを指定)");
			}

			// Getterが読むフィールドが、今のコードにあるか(名前を変えた・消したときに気づけるように)
			for (const auto& g : condition.getterNodes)
			{
				if (g.componentName.empty() || g.fieldName.empty()) continue; // 未設定はエディタで表示する
				if (!Reflection::FindField(g.componentName, g.fieldName))
				{
					error(what + u8" の条件が、存在しないフィールド「" + g.componentName + "." + g.fieldName + u8"」を読んでいます");
				}
			}

			// 決まりに合わないリンク(エディタで赤く表示されるもの)
			for (const auto& l : condition.links)
			{
				std::string reason;
				if (!condition.IsLinkValid(l, &reason))
				{
					error(what + u8" の条件に、決まりに合わないリンクがあります(リンクID " + std::to_string(l.id) + u8"): " + reason);
				}
			}
		};

	auto checkCrossfade = [&](const CrossfadeSetting& crossfade, const std::string& what)
		{
			if (crossfade.enabled && crossfade.duration <= 0.0f)
			{
				warning(what + u8" のクロスフェード時間が0以下です(クロスフェードしない扱いになります)");
			}
		};

	auto checkTransition = [&](const StateTransition& t, const std::string& fromName)
		{
			std::string what = fromName + u8" の遷移(ID " + std::to_string(t.id) + u8")";
			if (!FindState(t.toStateId))
			{
				error(what + u8" の行き先のStateがありません");
			}
			checkCommon(t.always, t.condition, what);
			checkCrossfade(t.crossfade, what);
		};

	for (const auto& t : entry.transitions) checkTransition(t, u8"Entry");
	for (const auto& s : states)
	{
		for (const auto& t : s.transitions) checkTransition(t, u8"「" + s.name + u8"」");
	}

	// Blend の中のチェックで使う
	const BlendValidateReport blendReport{ error, warning, checkCommon };

	// 子グラフ
	for (const auto& s : states)
	{
		// このStateに入ってくる遷移のID
		std::vector<int> incoming;
		for (const auto& t : entry.transitions)
		{
			if (t.toStateId == s.id) incoming.push_back(t.id);
		}
		for (const auto& from : states)
		{
			for (const auto& t : from.transitions)
			{
				if (t.toStateId == s.id) incoming.push_back(t.id);
			}
		}

		// エントリー → クリップ
		for (const auto& l : s.child.entryLinks)
		{
			std::string what = u8"「" + s.name + u8"」のエントリーのつながり(ID " + std::to_string(l.id) + u8")";
			if (std::find(incoming.begin(), incoming.end(), l.entryTransitionId) == incoming.end())
			{
				error(what + u8" に対応する、入ってくる遷移がありません");
			}
			if (s.child.GetNodeKind(l.toNodeId) == ChildNodeKind::None)
			{
				error(what + u8" の行き先のノード(Clip か Blend)がありません");
			}
			checkCommon(l.always, l.condition, what);
		}

		// つながりの無いエントリー
		for (int transitionId : incoming)
		{
			bool connected = std::any_of(s.child.entryLinks.begin(), s.child.entryLinks.end(),
				[transitionId](const EntryLink& l) { return l.entryTransitionId == transitionId; });
			if (!connected)
			{
				error(u8"「" + s.name + u8"」に、どのノードにもつながっていないエントリーがあります(遷移ID " + std::to_string(transitionId) + u8")");
			}
		}

		// クリップ → クリップ
		for (const auto& l : s.child.clipLinks)
		{
			std::string what = u8"「" + s.name + u8"」のクリップのつながり(ID " + std::to_string(l.id) + u8")";
			if (s.child.GetNodeKind(l.fromNodeId) == ChildNodeKind::None)
			{
				error(what + u8" の元のノード(Clip か Blend)がありません");
			}
			if (s.child.GetNodeKind(l.toNodeId) == ChildNodeKind::None)
			{
				error(what + u8" の行き先のノード(Clip か Blend)がありません");
			}
			checkCommon(l.always, l.condition, what);
			checkCrossfade(l.crossfade, what);
		}

		// Blend の中
		for (const auto& b : s.child.blends)
		{
			ValidateBlendGraph(b.graph, u8"「" + s.name + u8"」のBlend「" + b.name + u8"」", blendReport);
		}
	}

	return errorCount == 0; 
}

// JSON
nlohmann::ordered_json StateMachineData::ToJson() const
{
	nlohmann::ordered_json j;
	j["version"] = kVersion;
	j["nextId"] = nextId;

	// Entry
	nlohmann::ordered_json je;
	je["transitions"] = nlohmann::json::array();
	for (const auto& t : entry.transitions) je["transitions"].push_back(::ToJson(t));
	je["editor"] = PositionToJson(entry.editorPos);
	j["entry"] = je;

	// States
	j["states"] = nlohmann::json::array();
	for (const auto& s : states)
	{
		nlohmann::ordered_json js;
		js["id"] = s.id;
		js["name"] = s.name;

		js["transitions"] = nlohmann::json::array();
		for (const auto& t : s.transitions) js["transitions"].push_back(::ToJson(t));

		// 子グラフ
		nlohmann::ordered_json jc;
		jc["clips"] = nlohmann::json::array();
		for (const auto& c : s.child.clips)
		{
			nlohmann::ordered_json jclip;
			jclip["id"] = c.id;
			jclip["name"] = c.name;
			jclip["clip"] = c.clipName;
			jclip["loop"] = c.loop;
			jclip["rootMotion"] = c.rootMotion;
			jclip["editor"] = PositionToJson(c.editorPos);
			jc["clips"].push_back(jclip);
		}

		jc["blends"] = nlohmann::json::array();
		for (const auto& b : s.child.blends)
		{
			nlohmann::ordered_json jb;
			jb["id"] = b.id;
			jb["name"] = b.name;
			jb["loop"] = b.loop;
			jb["editor"] = PositionToJson(b.editorPos);
			jb["graph"] = BlendGraphToJson(b.graph);
			jc["blends"].push_back(jb);
		}

		jc["entryLinks"] = nlohmann::json::array();
		for (const auto& l : s.child.entryLinks)
		{
			nlohmann::ordered_json jl;
			jl["id"] = l.id;
			jl["entry"] = l.entryTransitionId;
			jl["to"] = l.toNodeId;
			WriteCondition(jl, l.always, l.condition);
			jc["entryLinks"].push_back(jl);
		}

		jc["clipLinks"] = nlohmann::json::array();
		for (const auto& l : s.child.clipLinks)
		{
			nlohmann::ordered_json jl;
			jl["id"] = l.id;
			jl["from"] = l.fromNodeId;
			jl["to"] = l.toNodeId;
			WriteCondition(jl, l.always, l.condition);
			WriteCrossfade(jl, l.crossfade);
			jc["clipLinks"].push_back(jl);
		}

		// エントリーのエディタ上の位置 (キーは、入ってくる遷移のID)
		nlohmann::ordered_json jentries = nlohmann::json::array();
		for (const auto& [transitionId, pos] : s.child.entryEditorPos)
		{
			nlohmann::ordered_json jp = PositionToJson(pos);
			jp["entry"] = transitionId;
			jentries.push_back(jp);
		}
		jc["editor"]["entries"] = jentries;

		js["child"] = jc;

		js["editor"] = PositionToJson(s.editorPos);
		js["editor"]["color"] = { s.editorColor[0], s.editorColor[1], s.editorColor[2] };

		j["states"].push_back(js);
	}

	return j;
}

bool StateMachineData::FromJson(const nlohmann::json& j, const std::string& sourceName)
{
	entry = StateMachineEntry{};
	states.clear();
	nextId = 1;

	int version = j.value("version", 0);
	if (version != kVersion)
	{
		Log(u8"[StateMachine] エラー(%s): 対応していない形式です(version %d)", sourceName.c_str(), version);
		return false;
	}

	// Entry
	if (j.contains("entry"))
	{
		const auto& je = j["entry"];
		if (je.contains("transitions"))
		{
			for (const auto& jt : je["transitions"]) entry.transitions.push_back(TransitionFromJson(jt));
		}
		if (je.contains("editor")) entry.editorPos = PositionFromJson(je["editor"]);
	}

	// States
	if (j.contains("states"))
	{
		for (const auto& js : j["states"])
		{
			StateNode s;
			s.id = js.value("id", -1);
			s.name = js.value("name", "");

			if (js.contains("transitions"))
			{
				for (const auto& jt : js["transitions"]) s.transitions.push_back(TransitionFromJson(jt));
			}

			if (js.contains("child"))
			{
				const auto& jc = js["child"];

				if (jc.contains("clips"))
				{
					for (const auto& jclip : jc["clips"])
					{
						ClipNode c;
						c.id = jclip.value("id", -1);
						c.name = jclip.value("name", "");
						c.clipName = jclip.value("clip", "");
						c.loop = jclip.value("loop", true);
						c.rootMotion = jclip.value("rootMotion", false);
						if (jclip.contains("editor")) c.editorPos = PositionFromJson(jclip["editor"]);
						s.child.clips.push_back(c);
					}
				}

				if (jc.contains("blends"))
				{
					for (const auto& jb : jc["blends"])
					{
						BlendNode b;
						b.id = jb.value("id", -1);
						b.name = jb.value("name", "");
						b.loop = jb.value("loop", true);
						if (jb.contains("editor")) b.editorPos = PositionFromJson(jb["editor"]);
						if (jb.contains("graph"))  BlendGraphFromJson(b.graph, jb["graph"]);
						s.child.blends.push_back(b);
					}
				}

				if (jc.contains("entryLinks"))
				{
					for (const auto& jl : jc["entryLinks"])
					{
						EntryLink l;
						l.id = jl.value("id", -1);
						l.entryTransitionId = jl.value("entry", -1);
						l.toNodeId = jl.value("to", -1);
						ReadCondition(jl, l.always, l.condition);
						s.child.entryLinks.push_back(l);
					}
				}

				if (jc.contains("clipLinks"))
				{
					for (const auto& jl : jc["clipLinks"])
					{
						ClipLink l;
						l.id = jl.value("id", -1);
						l.fromNodeId = jl.value("from", -1);
						l.toNodeId = jl.value("to", -1);
						ReadCondition(jl, l.always, l.condition);
						ReadCrossfade(jl, l.crossfade);
						s.child.clipLinks.push_back(l);
					}
				}

				if (jc.contains("editor") && jc["editor"].contains("entries"))
				{
					for (const auto& jp : jc["editor"]["entries"])
					{
						s.child.entryEditorPos[jp.value("entry", -1)] = PositionFromJson(jp);
					}
				}
			}

			if (js.contains("editor"))
			{
				const auto& jed = js["editor"];
				s.editorPos = PositionFromJson(jed);
				if (jed.contains("color") && jed["color"].is_array() && jed["color"].size() == 3)
				{
					s.editorColor[0] = jed["color"][0].get<float>();
					s.editorColor[1] = jed["color"][1].get<float>();
					s.editorColor[2] = jed["color"][2].get<float>();
				}
			}

			states.push_back(s);
		}
	}

	// 採番は、ファイルの値と、実際に使われている最大IDの大きいほうに合わせる
	int maxId = 0;
	for (const auto& t : entry.transitions) maxId = (std::max)(maxId, t.id);
	for (const auto& s : states)
	{
		maxId = (std::max)(maxId, s.id);
		for (const auto& t : s.transitions)      maxId = (std::max)(maxId, t.id);
		for (const auto& c : s.child.clips)      maxId = (std::max)(maxId, c.id);
		for (const auto& b : s.child.blends)     maxId = (std::max)(maxId, b.id);
		for (const auto& l : s.child.entryLinks) maxId = (std::max)(maxId, l.id);
		for (const auto& l : s.child.clipLinks)  maxId = (std::max)(maxId, l.id);
	}
	nextId = (std::max)(j.value("nextId", 1), maxId + 1);

	return Validate(sourceName);
}

// 編集用の検索
StateNode* StateMachineData::FindState(int stateId)
{
	return const_cast<StateNode*>(std::as_const(*this).FindState(stateId));
}
StateTransition* StateMachineData::FindTransition(int transitionId)
{
	return const_cast<StateTransition*>(std::as_const(*this).FindTransition(transitionId));
}

// 他のStateと重複しない名前を作る
std::string StateMachineData::MakeUniqueStateName(const std::string& baseName) const
{
	std::string base = baseName.empty() ? "State" : baseName;
	if (!FindStateByName(base)) return base;

	// 同じ名前があれば、末尾に番号を付ける("New State 1"、"New State 2"…)
	for (int i = 1; ; ++i)
	{
		std::string name = base + " " + std::to_string(i);
		if (!FindStateByName(name)) return name;
	}
}

// 遷移を持っているリスト(Entry、またはどれかのState)を探す
std::vector<StateTransition>* StateMachineData::FindTransitionOwner(int transitionId)
{
	for (const auto& t : entry.transitions)
	{
		if (t.id == transitionId) return &entry.transitions;
	}
	for (auto& s : states)
	{
		for (const auto& t : s.transitions)
		{
			if (t.id == transitionId) return &s.transitions;
		}
	}
	return nullptr;
}
