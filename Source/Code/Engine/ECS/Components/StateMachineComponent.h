#pragma once
#include <string>
#include "Engine/ECS/Reflect.h"

// Stateマシンの状態
struct StateMachineComponent
{
	REFLECT_BEGIN(StateMachineComponent, "State Machine")
		
		REFLECT_FIELD(std::string, graphPath,        (""), ("State Graph",   0.0f, 0.0f, 0.1f, "STATEGRAPH_PATH"))
		REFLECT_FIELD(std::string, currentStateName, (""), ("Current State", 0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient))
		REFLECT_FIELD(std::string, currentNodeName,  (""), ("Current Node",  0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 再生中のノード(Clip か Blend)

	REFLECT_END()

	int currentStateId = -1;      // 現在のState
	int currentNodeId = -1;       // 再生中のノード(ClipかBlendのID)
	int enteredTransitionId = -1; // 現在のStateに入ってきた遷移

	// Blend の中で、前のフレームに使った Output への線のID
	int blendOutputLinkId = -1;

	// Stateに入った直後で、子グラフのエントリーをまだ処理していないか
	bool pendingEntry = false;

	// このフレームにStateに入ったか
	bool enteredThisFrame = false;

	// 現在のStateが、指定した名前のStateか
	bool IsIn(const std::string& stateName) const { return currentStateName == stateName; }
};