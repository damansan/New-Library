#pragma once
#include <string>
#include <vector>
#include <functional>
#include <json.hpp>
#include "Engine/Data/ConditionGraphData.h"
#include "Engine/Data/StateMachineCommon.h"

// ブレンド比率をどこから取るか
enum class BlendRateSource
{
	Fixed,    // その場で決めた値
	Variable, // Component の変数から読む
};

// Blend の中のノードの種類
enum class BlendNodeKind
{
	None,     // 見つからない
	Output,   // 出力のノード
	Source,   // 材料(アニメーションの元)
	PairLink, // 2つの材料をブレンドする線
};

// 材料(クリップを1本再生する)
struct BlendSourceNode
{
	int id = -1;
	std::string name;     // 表示名
	std::string clipName;
	EditorPosition editorPos;
};

// ペア(材料2つ(A・B)を結ぶ線。比率で混ぜた結果を、線の真ん中から出す)
struct BlendPairLink
{
	int id = -1;
	int fromId = -1; // A(材料のID)
	int toId = -1;   // B(材料のID)

	BlendRateSource rateSource = BlendRateSource::Fixed;

	// その場で決めた値のときに使う(Bの割合。0 = A 100%、1 = B 100%)
	float fixedRate = 0.5f;

	// Component の変数から読むときに使う(条件グラフのゲッターと同じ持ち方)
	std::string componentName;
	std::string fieldName;
	FloatComponent selectedComponent = FloatComponent::None;
};

// 終着点(1つだけ)
struct BlendOutputNode
{
	int id = -1;
	EditorPosition editorPos{ 500.0f, 200.0f };
};

// Output への線(条件と優先度を持つ)
struct BlendOutputLink
{
	int id = -1;
	int fromId = -1; // 材料かペアのID
	bool always = false;
	ConditionGraph condition;
};

// Blend の中のグラフ
struct BlendGraph
{
	BlendOutputNode outputNode;
	std::vector<BlendSourceNode> sources;
	std::vector<BlendPairLink>   pairLinks;
	std::vector<BlendOutputLink> outputLinks; // 並び順がそのまま優先度

	// 次に割り当てるID(この Blend の中での採番)
	int nextId = 2;

	BlendGraph() { outputNode.id = 1; }

	int GenerateId() { return nextId++; }

	// 使用中のIDの最大値+1に、採番をそろえる(読み込み後などに呼ぶ)
	void RecalculateNextId();

	// 検索
	const BlendSourceNode* FindSource(int id) const;
	const BlendPairLink* FindPairLink(int id) const;
	const BlendOutputLink* FindOutputLink(int id) const;
	// 編集用の検索
	BlendSourceNode* FindSource(int id);
	BlendPairLink* FindPairLink(int id);
	BlendOutputLink* FindOutputLink(int id);

	// ノードの種類(Output への線の fromId が、材料かペアかを調べる)
	BlendNodeKind GetNodeKind(int id) const;

	// 材料
	int  AddSource(const EditorPosition& pos); // 作成
	void RemoveSource(int id);                 // 削除

	// ペア
	int  AddPairLink(int fromId, int toId); // 作成
	void RemovePairLink(int id);            // 削除

	// Output への線
	int  AddOutputLink(int fromId);           // 作成
	void RemoveOutputLink(int id);            // 削除
	bool MoveOutputLink(int id, int offset);  // 優先度の入れ替え
};

// JSONとの変換
nlohmann::ordered_json BlendGraphToJson(const BlendGraph& graph);
void BlendGraphFromJson(BlendGraph& graph, const nlohmann::json& j);

// 整合性チェックで、見つけた問題を知らせる先
struct BlendValidateReport
{
	std::function<void(const std::string&)> error;   // エラー(読み込み失敗扱い)
	std::function<void(const std::string&)> warning; // 警告(ログだけ)
	std::function<void(bool always, const ConditionGraph& condition, const std::string& what)> checkCondition; // 条件のチェック(遷移と同じもの)
};

// 整合性チェック
void ValidateBlendGraph(const BlendGraph& graph, const std::string& blendName, const BlendValidateReport& report);