#include "StateMachineData.h"
#include <algorithm>
#include <unordered_set>
#include "Engine/Core/Log.h"

namespace
{
	// エディタ上の位置
	nlohmann::ordered_json ToJson(const EditorPosition& p)
	{
		nlohmann::ordered_json j;
		j["x"] = p.x;
		j["y"] = p.y;
		return j;
	}
	EditorPosition PositionFromJson(const nlohmann::json& j)
	{
		EditorPosition p;
		p.x = j.value("x", 0.0f);
		p.y = j.value("y", 0.0f);
		return p;
	}

	// 遷移・つながりの共通部分(always / condition)
	void WriteCommon(nlohmann::ordered_json& j, bool always, const ConditionGraph& condition)
	{
		j["always"] = always;
		j["condition"] = ConditionGraphToJson(condition);
	}
	void ReadCommon(const nlohmann::json& j, bool& always, ConditionGraph& condition)
	{
		always = j.value("always", false);
		if (j.contains("condition")) ConditionGraphFromJson(condition, j["condition"]);
	}

	// ブレンドの設定
	void WriteBlend(nlohmann::ordered_json& j, const BlendSetting& blend)
	{
		j["blend"] = blend.enabled;
		j["blendDuration"] = blend.duration;
	}
	void ReadBlend(const nlohmann::json& j, BlendSetting& blend)
	{
		blend.enabled = j.value("blend", true);
		blend.duration = j.value("blendDuration", 0.2f);
	}

	// 遷移
	nlohmann::ordered_json ToJson(const StateTransition& t)
	{
		nlohmann::ordered_json j;
		j["id"] = t.id;
		j["to"] = t.toStateId;
		WriteCommon(j, t.always, t.condition);
		WriteBlend(j, t.blend);
		return j;
	}
	StateTransition TransitionFromJson(const nlohmann::json& j)
	{
		StateTransition t;
		t.id = j.value("id", -1);
		t.toStateId = j.value("to", -1);
		ReadCommon(j, t.always, t.condition);
		ReadBlend(j, t.blend);
		return t;
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
		};
	auto checkBlend = [&](const BlendSetting& blend, const std::string& what)
		{
			if (blend.enabled && blend.duration <= 0.0f)
			{
				warning(what + u8" のブレンド時間が0以下です(ブレンドしない扱いになります)");
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
			checkBlend(t.blend, what);
		};

	for (const auto& t : entry.transitions) checkTransition(t, u8"Entry");
	for (const auto& s : states)
	{
		for (const auto& t : s.transitions) checkTransition(t, u8"「" + s.name + u8"」");
	}

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
			if (!s.child.FindClip(l.toClipId))
			{
				error(what + u8" の行き先のクリップがありません");
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
				error(u8"「" + s.name + u8"」に、どのクリップにもつながっていないエントリーがあります(遷移ID " + std::to_string(transitionId) + u8")");
			}
		}

		// クリップ → クリップ
		for (const auto& l : s.child.clipLinks)
		{
			std::string what = u8"「" + s.name + u8"」のクリップのつながり(ID " + std::to_string(l.id) + u8")";
			if (!s.child.FindClip(l.fromClipId))
			{
				error(what + u8" の元のクリップがありません");
			}
			if (!s.child.FindClip(l.toClipId))
			{
				error(what + u8" の行き先のクリップがありません");
			}
			checkCommon(l.always, l.condition, what);
			checkBlend(l.blend, what);
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
	je["editor"] = ::ToJson(entry.editorPos);
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
			jclip["editor"] = ::ToJson(c.editorPos);
			jc["clips"].push_back(jclip);
		}

		jc["entryLinks"] = nlohmann::json::array();
		for (const auto& l : s.child.entryLinks)
		{
			nlohmann::ordered_json jl;
			jl["id"] = l.id;
			jl["entry"] = l.entryTransitionId;
			jl["to"] = l.toClipId;
			WriteCommon(jl, l.always, l.condition);
			jc["entryLinks"].push_back(jl);
		}

		jc["clipLinks"] = nlohmann::json::array();
		for (const auto& l : s.child.clipLinks)
		{
			nlohmann::ordered_json jl;
			jl["id"] = l.id;
			jl["from"] = l.fromClipId;
			jl["to"] = l.toClipId;
			WriteCommon(jl, l.always, l.condition);
			WriteBlend(jl, l.blend);
			jc["clipLinks"].push_back(jl);
		}

		// エントリーのエディタ上の位置 (キーは、入ってくる遷移のID)
		nlohmann::ordered_json jentries = nlohmann::json::array();
		for (const auto& [transitionId, pos] : s.child.entryEditorPos)
		{
			nlohmann::ordered_json jp = ::ToJson(pos);
			jp["entry"] = transitionId;
			jentries.push_back(jp);
		}
		jc["editor"]["entries"] = jentries;

		js["child"] = jc;

		js["editor"] = ::ToJson(s.editorPos);
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

				if (jc.contains("entryLinks"))
				{
					for (const auto& jl : jc["entryLinks"])
					{
						EntryLink l;
						l.id = jl.value("id", -1);
						l.entryTransitionId = jl.value("entry", -1);
						l.toClipId = jl.value("to", -1);
						ReadCommon(jl, l.always, l.condition);
						s.child.entryLinks.push_back(l);
					}
				}

				if (jc.contains("clipLinks"))
				{
					for (const auto& jl : jc["clipLinks"])
					{
						ClipLink l;
						l.id = jl.value("id", -1);
						l.fromClipId = jl.value("from", -1);
						l.toClipId = jl.value("to", -1);
						ReadCommon(jl, l.always, l.condition);
						ReadBlend(jl, l.blend);
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
		for (const auto& l : s.child.entryLinks) maxId = (std::max)(maxId, l.id);
		for (const auto& l : s.child.clipLinks)  maxId = (std::max)(maxId, l.id);
	}
	nextId = (std::max)(j.value("nextId", 1), maxId + 1);

	return Validate(sourceName);
}