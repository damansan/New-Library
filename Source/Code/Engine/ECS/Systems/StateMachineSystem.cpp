#include "StateMachineSystem.h"
#include "ConditionGraphEvaluator.h"
#include "Engine/Core/Log.h"
#include "Engine/ECS/Components/AnimatorComponent.h"
#include "Engine/ECS/Components/SequencerComponent.h"
#include "Engine/ECS/Components/StateMachineComponent.h"

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
			BlendSetting blend = via ? via->blend : BlendSetting{};

			bool played = false;
			for (const auto& l : state->child.entryLinks)
			{
				if (l.entryTransitionId != sm.enteredTransitionId) continue;
				if (!l.always && !ConditionGraphEvaluator::Evaluate(l.condition, registry, entity)) continue;

				const ClipNode* clip = state->child.FindClip(l.toClipId);
				if (!clip) continue;

				PlayClip(animator, *clip, blend, sequencer);
				sm.currentClipId = clip->id;
				sm.currentClipName = clip->name;
				played = true;
				break;
			}

			if (!played)
			{
				// 条件が成立するつながりが無かった(前のクリップを再生し続ける)
				Log(u8"[StateMachine] 「%s」に入りましたが、再生するクリップが決まりませんでした(遷移ID %d)",
					state->name.c_str(), sm.enteredTransitionId);
			}
			continue;
		}

		// State内：今のクリップから出るつながりを、並び順に調べる
		for (const auto& l : state->child.clipLinks)
		{
			if (l.fromClipId != sm.currentClipId) continue;
			if (!l.always && !ConditionGraphEvaluator::Evaluate(l.condition, registry, entity)) continue;

			const ClipNode* clip = state->child.FindClip(l.toClipId);
			if (!clip) continue;

			PlayClip(animator, *clip, l.blend, sequencer);
			sm.currentClipId = clip->id;
			sm.currentClipName = clip->name;
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
void StateMachineSystem::PlayClip(AnimatorComponent& animator, const ClipNode& clip, const BlendSetting& blend, SequencerComponent* sequencer)
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

	float blendDuration = (blend.enabled && blend.duration > 0.0f) ? blend.duration : 0.0f;

	// ブレンドしない
	if (animator.currentAnimation.empty() || blendDuration <= 0.0f)
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
		animator.isBlending = false;
		animator.blendSnapshotPending = false;
		return;
	}

	// ブレンド開始
	animator.blendSnapshotPending = true;
	animator.blendTime = blendDuration;
	animator.blendRate = 0.0f;
	animator.isBlending = true;

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