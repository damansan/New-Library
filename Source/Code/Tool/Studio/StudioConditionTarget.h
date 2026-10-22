#pragma once

// 条件ウィンドウで編集する条件がどのリンクから来たものか
struct ConditionTarget
{
	enum class Kind
	{
		None,            // 何も編集していない
		StateLink,       // 親のリンク(エントリー → State ・ State → State)
		EntryLink,       // 子のリンク(エントリー → ノード)
		ClipLink,        // 子のリンク(ノード → ノード)
		BlendOutputLink, // Blend の中のリンク(材料かペア → Output)
	};

	Kind kind = Kind::None;
	int stateId = -1; // 子・Blend のリンクのとき：どのStateか
	int linkId = -1;
	int blendId = -1; // Blend のリンクのとき：どのBlendか

	bool IsValid() const { return kind != Kind::None; }
	bool operator==(const ConditionTarget& o) const
	{
		return kind == o.kind && stateId == o.stateId && linkId == o.linkId && blendId == o.blendId;
	}
	bool operator!=(const ConditionTarget& o) const { return !(*this == o); }
};