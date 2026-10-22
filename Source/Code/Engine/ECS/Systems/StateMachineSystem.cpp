#include "StateMachineSystem.h"
#include "ConditionGraphEvaluator.h"
#include "Engine/Core/Log.h"
#include "Engine/ECS/Components/AnimatorComponent.h"
#include "Engine/ECS/Components/SequencerComponent.h"
#include "Engine/ECS/Components/StateMachineComponent.h"

namespace
{
	// クロスフェードを始める
	void StartCrossfade(AnimatorComponent& animator, const CrossfadeSetting& crossfade)
	{
		const float duration = (crossfade.enabled && crossfade.duration > 0.0f) ? crossfade.duration : 0.0f;
		const bool nothingPlayedYet = animator.currentAnimation.empty() && animator.playMode == AnimPlayMode::Clip;

		if (nothingPlayedYet || duration <= 0.0f)
		{
			animator.isCrossfading = false;
			animator.crossfadeSnapshotPending = false;
			return;
		}

		// 混ぜる元のポーズは、次の AnimationSystem の更新で保存する
		animator.crossfadeSnapshotPending = true;
		animator.crossfadeTime = duration;
		animator.crossfadeRate = 0.0f;
		animator.isCrossfading = true;
	}
}

// グラフを取得する
const StateMachineData* StateMachineSystem::Load(const std::string& path)
{
	LoadedStateMachine& loaded = graphCache.GetOrLoad(path,
		[this, &path](LoadedStateMachine& l, const nlohmann::json& j)
		{
			l.loaded = true;
			l.valid = l.data.FromJson(j, path);

			// 読み込み直したら、このパスの「Stateが無い」の報告済みを消して、確かめ直せるようにする
			std::string prefix = path + "\n";
			for (auto it = reportedMissingStates.begin(); it != reportedMissingStates.end(); )
			{
				if (it->compare(0, prefix.size(), prefix) == 0) it = reportedMissingStates.erase(it);
				else ++it;
			}
		});

	if (!loaded.loaded || !loaded.valid)
	{
		// 同じパスについては1回だけ報告する
		if (reportedPaths.insert(path).second)
		{
			Log(u8"[StateMachine] %s を使用できません(読み込みに失敗したか、検証でエラーがあります)", path.c_str());
		}
		return nullptr;
	}

	// 直った後に、再び壊れたときにも報告できるようにする
	reportedPaths.erase(path);
	return &loaded.data;
}

// 条件が成立した最初の遷移を返す(優先度順)
const StateTransition* StateMachineSystem::FindPassedTransition(
	const std::vector<StateTransition>& transitions, entt::registry& registry, entt::entity entity)
{
	for (const auto& t : transitions)
	{
		if (t.always || ConditionGraphEvaluator::Evaluate(t.condition, registry, entity))
		{
			return &t;
		}
	}
	return nullptr;
}

// Stateに入る
void StateMachineSystem::EnterState(StateMachineComponent& sm, const StateNode& state, const StateTransition& via)
{
	sm.currentStateId = state.id;
	sm.currentStateName = state.name;
	sm.enteredTransitionId = via.id;
	sm.enteredThisFrame = true;

	// 最初のクリップは、ゲームプレイの各Systemが動いた後に決める
	sm.pendingEntry = true;
}

// ノードを再生する
bool StateMachineSystem::PlayNode(StateMachineComponent& sm, AnimatorComponent& animator, const ChildGraph& child, int nodeId, const CrossfadeSetting& crossfade, SequencerComponent* sequencer)
{
	switch (child.GetNodeKind(nodeId))
	{
	case ChildNodeKind::Clip:
	{
		const ClipNode* clip = child.FindClip(nodeId);
		PlayClip(animator, *clip, crossfade, sequencer);
		sm.currentNodeId = clip->id;
		sm.currentNodeName = clip->name;
		return true;
	}

	case ChildNodeKind::Blend:
	{
		const BlendNode* blend = child.FindBlend(nodeId);
		PlayBlend(animator, *blend, crossfade);
		sm.currentNodeId = blend->id;
		sm.currentNodeName = blend->name;
		sm.blendOutputLinkId = -1; // Blend に入った直後は、前の線が無い
		return true;
	}

	default:
		return false; // 見つからない
	}
}

// Stateの遷移を決める
void StateMachineSystem::UpdateState(entt::registry& registry)
{
	auto view = registry.view<StateMachineComponent>();
	for (auto entity : view)
	{
		auto& sm = view.get<StateMachineComponent>(entity);

		sm.enteredThisFrame = false;

		if (sm.graphPath.empty()) continue;

		const StateMachineData* data = Load(sm.graphPath);
		if (!data) continue;

		const StateNode* current = data->FindState(sm.currentStateId);

		// 未初期化、またはグラフの作り直しで今のStateが無くなった場合は、Entryから入り直す
		if (!current)
		{
			// このエンティティを処理するSystemが使うStateが、グラフにあるかを確かめる
			CheckRequiredStates(*data, sm.graphPath, registry, entity);

			const StateTransition* t = FindPassedTransition(data->entry.transitions, registry, entity);
			if (!t) continue;

			if (const StateNode* next = data->FindState(t->toStateId))
			{
				EnterState(sm, *next, *t);
			}
			continue;
		}

		// 今のStateから出る遷移を、優先度順に調べる
		const StateTransition* t = FindPassedTransition(current->transitions, registry, entity);
		if (!t) continue;

		if (const StateNode* next = data->FindState(t->toStateId))
		{
			EnterState(sm, *next, *t);
		}
	}
}

// 再生するクリップを決める
void StateMachineSystem::UpdateAnimation(entt::registry& registry)
{
	auto view = registry.view<StateMachineComponent, AnimatorComponent>();
	for (auto entity : view)
	{
		auto& sm = view.get<StateMachineComponent>(entity);
		auto& animator = view.get<AnimatorComponent>(entity);
		if (sm.graphPath.empty()) continue;

		const StateMachineData* data = Load(sm.graphPath);
		if (!data) continue;

		const StateNode* state = data->FindState(sm.currentStateId);
		if (!state) continue;

		SequencerComponent* sequencer = registry.try_get<SequencerComponent>(entity);

		// Stateに入った直後：入ってきた遷移のエントリーから、最初のクリップを決める
		if (sm.pendingEntry)
		{
			sm.pendingEntry = false;

			const StateTransition* via = data->FindTransition(sm.enteredTransitionId);
			CrossfadeSetting crossfade = via ? via->crossfade : CrossfadeSetting{};

			bool played = false;
			for (const auto& l : state->child.entryLinks)
			{
				if (l.entryTransitionId != sm.enteredTransitionId) continue;
				if (!l.always && !ConditionGraphEvaluator::Evaluate(l.condition, registry, entity)) continue;

				if (!PlayNode(sm, animator, state->child, l.toNodeId, crossfade, sequencer)) continue;

				played = true;
				break;
			}

			if (!played)
			{
				// 条件が成立するつながりが無かった
				Log(u8"[StateMachine] 「%s」に入りましたが、再生するノードが決まりませんでした(遷移ID %d)", 
					state->name.c_str(), sm.enteredTransitionId);
			}
			continue;
		}

		// State内：今のクリップから出るつながりを、並び順に調べる
		for (const auto& l : state->child.clipLinks)
		{
			if (l.fromNodeId != sm.currentNodeId) continue;
			if (!l.always && !ConditionGraphEvaluator::Evaluate(l.condition, registry, entity)) continue;

			if (!PlayNode(sm, animator, state->child, l.toNodeId, l.crossfade, sequencer)) continue;
			break;
		}
	}
}

// 申告されたState名の一覧
std::vector<StateMachineSystem::RequiredState>& StateMachineSystem::GetRequiredStates()
{
	static std::vector<RequiredState> requiredStates;
	return requiredStates;
}

// 申告されたState名が、このエンティティのグラフにあるかを確かめる
void StateMachineSystem::CheckRequiredStates(const StateMachineData& data, const std::string& path, const entt::registry& registry, entt::entity entity)
{
	for (const auto& r : GetRequiredStates())
	{
		// 申告したSystemの対象でないエンティティ(Playerの申告に対するEnemyなど)は確かめない
		if (r.usesState && !r.usesState(registry, entity)) continue;
		if (data.FindStateByName(r.stateName)) continue;

		// 同じ組については1回だけ報告する
		std::string key = path + "\n" + r.stateName + "\n" + r.user;
		if (!reportedMissingStates.insert(key).second) continue;

		Log(u8"[StateMachine] 警告(%s): State「%s」がグラフにありません(%s が使用)",
			path.c_str(), r.stateName.c_str(), r.user.c_str());
	}
}

// 申告を一覧に足す
void StateMachineSystem::AddRequiredState(const std::string& stateName, const std::string& user, UsesStateFunc usesState)
{
	auto& list = GetRequiredStates();
	for (const auto& r : list)
	{
		if (r.stateName == stateName && r.user == user) return; // 申告済み
	}
	list.push_back({ stateName, user, usesState });
}

// Blend を再生する
void StateMachineSystem::PlayBlend(AnimatorComponent& animator, const BlendNode& blend, const CrossfadeSetting& crossfade)
{
	// クロスフェードを始める
	StartCrossfade(animator, crossfade);

	animator.playMode = AnimPlayMode::Blend;
	animator.blendPhase = 0.0f;
	animator.isPlaying = true;
	animator.isEnd = false;
	animator.loop = blend.loop;
	animator.useRootMotion = false; // Blend ではルートモーションを使わない

	// 材料と比率は、毎フレームの評価(B2-3)で決める。それまでは空にしておく
	animator.blendClipA.clear();
	animator.blendClipB.clear();
	animator.blendWeight = 0.0f;
	animator.blendTrimStartA = 0.0f;
	animator.blendTrimEndA = 1.0f;
	animator.blendTrimStartB = 0.0f;
	animator.blendTrimEndB = 1.0f;
}

// トリミング範囲を探す
bool StateMachineSystem::FindTrimmingRange(SequencerData& data, const std::string& clipName, float& outStart, float& outEnd)
{
	for (auto& clip : data.clips)
	{
		if (clip.clipName != clipName) continue;
		for (auto& range : clip.ranges)
		{
			if (range.flag == SequencerFlag::Trimming)
			{
				outStart = range.start;
				outEnd = range.end;
				return true;
			}
		}
		break;
	}
	return false;
}

// クリップを再生する
void StateMachineSystem::PlayClip(AnimatorComponent& animator, const ClipNode& clip, const CrossfadeSetting& crossfade, SequencerComponent* sequencer)
{
	// トリミング範囲を調べる
	float trimStart = 0.0f;
	float trimEnd = -1.0f;
	if (sequencer && !sequencer->sequencerPath.empty())
	{
		SequencerData& data = trimDataCache.GetOrLoad(sequencer->sequencerPath,
			[](SequencerData& data, const nlohmann::json& j) { data.FromJson(j); });

		float normStart = 0.0f, normEnd = 1.0f;
		if (FindTrimmingRange(data, clip.clipName, normStart, normEnd))
		{
			trimStart = normStart;
			trimEnd = normEnd;
		}
	}

	float crossfadeDuration = (crossfade.enabled && crossfade.duration > 0.0f) ? crossfade.duration : 0.0f;

	// クロスフェードしない
	if (animator.currentAnimation.empty() || crossfadeDuration <= 0.0f) 
	{
		animator.currentAnimation = clip.clipName;
		animator.trimStart = trimStart;
		animator.trimEnd = trimEnd;
		animator.trimResolved = false;
		animator.animationTime = 0.0f;
		animator.isPlaying = true;
		animator.isEnd = false;
		animator.useRootMotion = clip.rootMotion;
		animator.loop = clip.loop;
		animator.isCrossfading = false;
		animator.crossfadeSnapshotPending = false;
		return;
	}

	// クロスフェードを始める
	StartCrossfade(animator, crossfade);

	animator.playMode = AnimPlayMode::Clip;
	animator.currentAnimation = clip.clipName;
	animator.trimStart = trimStart;
	animator.trimEnd = trimEnd;
	animator.trimResolved = false;
	animator.animationTime = 0.0f;
	animator.isPlaying = true;
	animator.isEnd = false;
	animator.useRootMotion = clip.rootMotion;
	animator.loop = clip.loop;
}