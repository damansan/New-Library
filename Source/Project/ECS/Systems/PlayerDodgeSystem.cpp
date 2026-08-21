#include "PlayerDodgeSystem.h"

#include "Project/ECS/Components/GameInputComponent.h"
#include "Project/ECS/Components/PlayerDodgeComponent.h"
#include "Engine/ECS/Components/MovementComponent.h"
#include "Engine/ECS/Components/AnimatorComponent.h"
#include "Engine/ECS/Components/SequencerComponent.h"

void PlayerDodgeSystem::Update(entt::registry& registry, float elapsedTime, const SceneContext& context)
{
	auto view = registry.view<GameInputComponent, PlayerDodgeComponent, MovementComponent, AnimatorComponent, SequencerComponent>();

	view.each([&](auto entity,
		GameInputComponent& input,
		PlayerDodgeComponent& dodge,
		MovementComponent& movement,
		AnimatorComponent& animator,
		SequencerComponent& sequencer)
		{
			// 既に押している間は「離した判定ライン」を下回るまで押している扱いにする(ヒステリシス)
			bool triggerHeld = dodge.wasTriggerHeld
				? (input.dodge > dodge.triggerReleaseThreshold)
				: (input.dodge >= dodge.triggerThreshold);

			bool triggerPressed = (triggerHeld && !dodge.wasTriggerHeld);

			if (dodge.state == PlayerDodgeState::None)
			{
				if (triggerPressed)
				{
					dodge.state = PlayerDodgeState::Dodging;
					movement.xzControlLocked = true; // 移動の制御をロック
				}
			}
			else // Dodging
			{
				// animExitがTrue、さらにトリガーが離されていれば早期終了
				bool exitRequested = sequencer.animExit && !triggerHeld;

				if (animator.isEnd || exitRequested)
				{
					dodge.state = PlayerDodgeState::None;
					movement.xzControlLocked = false; // 移動の制御を解除
				}
			}

			// 状態に関わらず、毎フレーム必ず更新する
			dodge.wasTriggerHeld = triggerHeld;
			dodge.isDodging = (dodge.state == PlayerDodgeState::Dodging);
		});
}