#pragma once
#include <set>
#include <string>
#include "entt.hpp"
#include "Engine/Core/JsonFileCache.h"
#include "Engine/Data/StateMachineData.h"
#include "Engine/Data/SequencerData.h"

struct AnimatorComponent;
struct SequencerComponent;
struct StateMachineComponent;

// 読み込んだグラフと、その結果
struct LoadedStateMachine
{
	StateMachineData data;
	bool loaded = false; // JSONとして読み込めたか
	bool valid = false;  // 検証を通ったか
};

// Stateマシン(Stateの遷移と、アニメーションの切り替え)
class StateMachineSystem
{
public:
	// Stateの遷移を決める (ゲームプレイの各Systemより前に呼ぶ)
	void UpdateState(entt::registry& registry);

	// 再生するクリップを決める (ゲームプレイの各Systemより前に呼ぶ)
	void UpdateAnimation(entt::registry& registry);

	// 使用するState名を申告する(グラフの読み込み時に、存在するかを確かめる)
	static void RequireState(const std::string& stateName, const std::string& user);

private:
	
	// 申告されたState名と、申告した側の名前
	struct RequiredState
	{
		std::string stateName;
		std::string user;
	};
	static std::vector<RequiredState>& GetRequiredStates();

	// 申告されたState名が、グラフにあるかを確かめる(無ければ警告)
	static void CheckRequiredStates(const StateMachineData& data, const std::string& path);

	// グラフを取得する (読み込めない・検証エラーの場合はnullptr)
	const StateMachineData* Load(const std::string& path);

	// 条件が成立した最初の遷移を返す(優先度順)
	static const StateTransition* FindPassedTransition(
		const std::vector<StateTransition>& transitions, entt::registry& registry, entt::entity entity);

	// Stateに入る
	static void EnterState(StateMachineComponent& sm, const StateNode& state, const StateTransition& via);

	// クリップを再生する
	void PlayClip(AnimatorComponent& animator, const ClipNode& clip, const BlendSetting& blend, SequencerComponent* sequencer);

	static bool FindTrimmingRange(SequencerData& data, const std::string& clipName, float& outStart, float& outEnd);

	JsonFileCache<LoadedStateMachine> graphCache;
	JsonFileCache<SequencerData> trimDataCache;

	// 使えないことをLogに報告済みのパス
	std::set<std::string> reportedPaths;
};