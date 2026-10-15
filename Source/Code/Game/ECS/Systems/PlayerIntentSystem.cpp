#include "PlayerIntentSystem.h"
#include <algorithm>

#include "Game/ECS/Components/GameInputComponent.h"
#include "Game/ECS/Components/PlayerIntentComponent.h"
#include "Game/ECS/Systems/PlayerMovementUtils.h"

void PlayerIntentSystem::Update(entt::registry& registry, float elapsedTime, const SceneContext& context)
{
	// カメラがない場合は処理しない
	if (!context.camera)return;

	// カメラの前方向と右方向を取得
	const DirectX::XMFLOAT3 front = context.camera->GetFront();
	const DirectX::XMFLOAT3 right = context.camera->GetRight();

	auto view = registry.view<GameInputComponent, PlayerIntentComponent>();
	view.each([&](const GameInputComponent& input, PlayerIntentComponent& intent)
		{
			// 生の入力を、カメラ基準のワールド方向に変換
			auto dir = PlayerMovementUtils::StickToWorldVelocity(input.moveStick.x, input.moveStick.y, front, right, 1.0f);
			intent.hasRawInput = (dir.inputLength > 0.0f);

			// 動きたいか(猶予付き)
			if (intent.hasRawInput)
			{
				// 入力あり：方向と倒し具合を更新して、猶予をリセット
				intent.moveDir = dir.velocity; // 長さ1
				intent.moveAmount = (std::min)(dir.inputLength, 1.0f);
				intent.graceTimer = 0.0f;
				intent.wantsMove = true;
			}
			else
			{
				// 入力なし：猶予の間は、最後の方向と倒し具合を保つ
				intent.graceTimer += elapsedTime;
				if (intent.graceTimer >= intent.moveGraceTime)
				{
					intent.wantsMove = false;
					intent.moveAmount = 0.0f;
				}
			}

			// 走りたいか(ヒステリシス)
			if (!intent.wantsMove)
			{
				intent.wantsRun = false;
			}
			else if (intent.moveAmount >= intent.runEnterThreshold)
			{
				intent.wantsRun = true;
			}
			else if (intent.moveAmount < intent.runExitThreshold)
			{
				intent.wantsRun = false;
			}
		});
}