#include "StateMachineBlendData.h"
#include <algorithm>
#include <unordered_set>
#include <utility>

using namespace StateMachineCommon;

namespace
{
	// 成分(X/Y/Z/W)の文字列変換(条件グラフの保存と同じ文字列)
	std::string FloatComponentToString(FloatComponent c)
	{
		switch (c)
		{
		case FloatComponent::X: return "X";
		case FloatComponent::Y: return "Y";
		case FloatComponent::Z: return "Z";
		case FloatComponent::W: return "W";
		default:                return "None";
		}
	}
	FloatComponent FloatComponentFromString(const std::string& s)
	{
		if (s == "X") return FloatComponent::X;
		if (s == "Y") return FloatComponent::Y;
		if (s == "Z") return FloatComponent::Z;
		if (s == "W") return FloatComponent::W;
		return FloatComponent::None;
	}

	// 比率の出どころの文字列変換
	std::string BlendRateSourceToString(BlendRateSource s)
	{
		return (s == BlendRateSource::Variable) ? "Variable" : "Fixed";
	}
	BlendRateSource BlendRateSourceFromString(const std::string& s)
	{
		return (s == "Variable") ? BlendRateSource::Variable : BlendRateSource::Fixed;
	}
}

// 検索
const BlendSourceNode* BlendGraph::FindSource(int id) const 
{ 
	return FindById(sources, id); 
}
const BlendPairLink* BlendGraph::FindPairLink(int id) const 
{
	return FindById(pairLinks, id);
}
const BlendOutputLink* BlendGraph::FindOutputLink(int id) const 
{
	return FindById(outputLinks, id); 
}
// 編集用の検索
BlendSourceNode* BlendGraph::FindSource(int id)
{
	return const_cast<BlendSourceNode*>(std::as_const(*this).FindSource(id));
}
BlendPairLink* BlendGraph::FindPairLink(int id)
{
	return const_cast<BlendPairLink*>(std::as_const(*this).FindPairLink(id));
}
BlendOutputLink* BlendGraph::FindOutputLink(int id)
{
	return const_cast<BlendOutputLink*>(std::as_const(*this).FindOutputLink(id));
}

// ノードの種類
BlendNodeKind BlendGraph::GetNodeKind(int id) const
{
	if (id == outputNode.id)     return BlendNodeKind::Output;
	if (FindById(sources, id))   return BlendNodeKind::Source;
	if (FindById(pairLinks, id)) return BlendNodeKind::PairLink;
	return BlendNodeKind::None;
}

// 材料：作成
int BlendGraph::AddSource(const EditorPosition& pos)
{
	BlendSourceNode s;
	s.id = GenerateId();
	s.editorPos = pos;
	sources.push_back(s);
	return s.id;
}
// 材料：削除
void BlendGraph::RemoveSource(int id)
{
	// この材料を使っているペアを消す(ペアから Output への線も一緒に消える)
	std::vector<int> usedPairs;
	for (const auto& p : pairLinks)
	{
		if (p.fromId == id || p.toId == id) usedPairs.push_back(p.id);
	}
	for (int pairId : usedPairs)
	{
		RemovePairLink(pairId);
	}

	// この材料から Output への線を消す
	outputLinks.erase(
		std::remove_if(outputLinks.begin(), outputLinks.end(),
			[id](const BlendOutputLink& l) { return l.fromId == id; }),
		outputLinks.end());

	// 材料本体を消す
	sources.erase(
		std::remove_if(sources.begin(), sources.end(),
			[id](const BlendSourceNode& s) { return s.id == id; }),
		sources.end());
}

// ペア：作成
int BlendGraph::AddPairLink(int fromId, int toId)
{
	// A・B は、別々の材料である必要がある
	if (GetNodeKind(fromId) != BlendNodeKind::Source || GetNodeKind(toId) != BlendNodeKind::Source) return -1;
	if (fromId == toId) return -1;

	// 同じ2つの材料のペアは、向きに関係なく1本だけ
	for (const auto& p : pairLinks)
	{
		bool same = (p.fromId == fromId && p.toId == toId) || (p.fromId == toId && p.toId == fromId);
		if (same) return -1;
	}

	BlendPairLink p;
	p.id = GenerateId();
	p.fromId = fromId;
	p.toId = toId;
	pairLinks.push_back(p);
	return p.id;
}
// ペア：削除
void BlendGraph::RemovePairLink(int id)
{
	// このペアから Output への線を消す
	outputLinks.erase(
		std::remove_if(outputLinks.begin(), outputLinks.end(),
			[id](const BlendOutputLink& l) { return l.fromId == id; }),
		outputLinks.end());

	// ペア本体を消す
	pairLinks.erase(
		std::remove_if(pairLinks.begin(), pairLinks.end(),
			[id](const BlendPairLink& p) { return p.id == id; }),
		pairLinks.end());
}

// Output への線：作成
int BlendGraph::AddOutputLink(int fromId)
{
	// 元は、材料かペア
	BlendNodeKind kind = GetNodeKind(fromId);
	if (kind != BlendNodeKind::Source && kind != BlendNodeKind::PairLink) return -1;

	BlendOutputLink l;
	l.id = GenerateId();
	l.fromId = fromId;
	outputLinks.push_back(l); // 一番下 = 優先度が一番低い
	return l.id;
}
// Output への線：削除
void BlendGraph::RemoveOutputLink(int id)
{
	outputLinks.erase(
		std::remove_if(outputLinks.begin(), outputLinks.end(),
			[id](const BlendOutputLink& l) { return l.id == id; }),
		outputLinks.end());
}
// Output への線：優先度の入れ替え
bool BlendGraph::MoveOutputLink(int id, int offset)
{
	int index = -1;
	for (int i = 0; i < static_cast<int>(outputLinks.size()); ++i)
	{
		if (outputLinks[i].id == id) index = i;
	}

	int newIndex = index + offset;
	if (index < 0 || newIndex < 0 || newIndex >= static_cast<int>(outputLinks.size())) return false;

	std::swap(outputLinks[index], outputLinks[newIndex]);
	return true;
}

// 使用中のIDの最大値+1に、採番をそろえる
void BlendGraph::RecalculateNextId()
{
	int maxId = outputNode.id;
	for (const auto& s : sources)     maxId = (std::max)(maxId, s.id);
	for (const auto& p : pairLinks)   maxId = (std::max)(maxId, p.id);
	for (const auto& l : outputLinks) maxId = (std::max)(maxId, l.id);
	nextId = maxId + 1;
}

// 保存と読み込み
nlohmann::ordered_json BlendGraphToJson(const BlendGraph& g)
{
	nlohmann::ordered_json j;
	j["output"]["editor"] = PositionToJson(g.outputNode.editorPos);

	j["sources"] = nlohmann::json::array();
	for (const auto& s : g.sources)
	{
		nlohmann::ordered_json js;
		js["id"] = s.id;
		js["name"] = s.name;
		js["clip"] = s.clipName;
		js["editor"] = PositionToJson(s.editorPos);
		j["sources"].push_back(js);
	}

	j["pairLinks"] = nlohmann::json::array();
	for (const auto& p : g.pairLinks)
	{
		nlohmann::ordered_json jp;
		jp["id"] = p.id;
		jp["from"] = p.fromId;
		jp["to"] = p.toId;
		jp["rateSource"] = BlendRateSourceToString(p.rateSource);
		jp["fixedRate"] = p.fixedRate;
		jp["componentName"] = p.componentName;
		jp["fieldName"] = p.fieldName;
		jp["selectedComponent"] = FloatComponentToString(p.selectedComponent);
		j["pairLinks"].push_back(jp);
	}

	j["outputLinks"] = nlohmann::json::array();
	for (const auto& l : g.outputLinks)
	{
		nlohmann::ordered_json jl;
		jl["id"] = l.id;
		jl["from"] = l.fromId;
		WriteCondition(jl, l.always, l.condition);
		j["outputLinks"].push_back(jl);
	}

	return j;
}

void BlendGraphFromJson(BlendGraph& g, const nlohmann::json& j)
{
	// 前の中身が残らないように、空のグラフから読む
	g = BlendGraph{};

	if (j.contains("output") && j["output"].contains("editor"))
	{
		g.outputNode.editorPos = PositionFromJson(j["output"]["editor"]);
	}

	if (j.contains("sources"))
	{
		for (const auto& js : j["sources"])
		{
			BlendSourceNode s;
			s.id = js.value("id", -1);
			s.name = js.value("name", "");
			s.clipName = js.value("clip", "");
			if (js.contains("editor")) s.editorPos = PositionFromJson(js["editor"]);
			g.sources.push_back(s);
		}
	}

	if (j.contains("pairLinks"))
	{
		for (const auto& jp : j["pairLinks"])
		{
			BlendPairLink p;
			p.id = jp.value("id", -1);
			p.fromId = jp.value("from", -1);
			p.toId = jp.value("to", -1);
			p.rateSource = BlendRateSourceFromString(jp.value("rateSource", "Fixed"));
			p.fixedRate = jp.value("fixedRate", 0.5f);
			p.componentName = jp.value("componentName", "");
			p.fieldName = jp.value("fieldName", "");
			p.selectedComponent = FloatComponentFromString(jp.value("selectedComponent", "None"));
			g.pairLinks.push_back(p);
		}
	}

	if (j.contains("outputLinks"))
	{
		for (const auto& jl : j["outputLinks"])
		{
			BlendOutputLink l;
			l.id = jl.value("id", -1);
			l.fromId = jl.value("from", -1);
			ReadCondition(jl, l.always, l.condition);
			g.outputLinks.push_back(l);
		}
	}

	// 読み込んだIDの続きから採番する
	g.RecalculateNextId();
}


// 整合性チェック
void ValidateBlendGraph(const BlendGraph& g, const std::string& blendName, const BlendValidateReport& report)
{
	// 中のID(Blend ごとの番号。Output は 1 で固定)
	std::unordered_set<int> innerIds{ g.outputNode.id };
	auto checkInnerId = [&](int id, const std::string& what)
		{
			if (id < 0)
			{
				report.error(what + u8" のIDが設定されていません");
			}
			else if (!innerIds.insert(id).second)
			{
				report.error(what + u8" のID " + std::to_string(id) + u8" が重複しています");
			}
		};

	// 材料
	for (const auto& src : g.sources)
	{
		std::string what = blendName + u8" の材料(ID " + std::to_string(src.id) + u8")";
		checkInnerId(src.id, what);
		if (src.clipName.empty())
		{
			report.warning(what + u8" のクリップが選ばれていません");
		}
	}

	// ペアの線
	for (const auto& p : g.pairLinks)
	{
		std::string what = blendName + u8" のペア(ID " + std::to_string(p.id) + u8")";
		checkInnerId(p.id, what);

		if (g.GetNodeKind(p.fromId) != BlendNodeKind::Source || g.GetNodeKind(p.toId) != BlendNodeKind::Source)
		{
			report.error(what + u8" のAとBは、材料である必要があります");
		}
		else if (p.fromId == p.toId)
		{
			report.error(what + u8" のAとBが同じ材料です");
		}

		if (p.rateSource == BlendRateSource::Fixed)
		{
			if (p.fixedRate < 0.0f || p.fixedRate > 1.0f)
			{
				report.warning(what + u8" の比率が0～1の外です(実行時は端に寄せます)");
			}
		}
		else
		{
			// 変数から読む：読めるフィールドか
			if (p.componentName.empty() || p.fieldName.empty())
			{
				report.error(what + u8" の比率の変数が設定されていません");
			}
			else if (const FieldEntry* f = Reflection::FindField(p.componentName, p.fieldName))
			{
				if (f->type == ValueType::Unsupported)
				{
					report.error(what + u8" の比率の変数「" + p.componentName + "." + p.fieldName + u8"」は、比率として読めない型です");
				}
				else if ((f->type == ValueType::Float2 || f->type == ValueType::Float3 || f->type == ValueType::Float4) &&
					p.selectedComponent == FloatComponent::None)
				{
					report.error(what + u8" の比率の変数「" + p.componentName + "." + p.fieldName + u8"」は、使う成分(X/Y/Z/W)が選ばれていません");
				}
			}
			else
			{
				report.error(what + u8" の比率が、存在しないフィールド「" + p.componentName + "." + p.fieldName + u8"」を読んでいます");
			}
		}
	}

	// Output への線
	for (const auto& l : g.outputLinks)
	{
		std::string what = blendName + u8" のOutputへの線(ID " + std::to_string(l.id) + u8")";
		checkInnerId(l.id, what);

		BlendNodeKind kind = g.GetNodeKind(l.fromId);
		if (kind != BlendNodeKind::Source && kind != BlendNodeKind::PairLink)
		{
			report.error(what + u8" の元は、材料かペアである必要があります");
		}
		report.checkCondition(l.always, l.condition, what);
	}

	// Output に何もつながっていない
	if (g.outputLinks.empty())
	{
		report.error(blendName + u8" のOutputに何もつながっていません(再生するものがありません)");
	}
}