#include "PlayerIntentSystem.h"
#include <algorithm>

#include "Game/ECS/Components/GameInputComponent.h"
#include "Game/ECS/Components/PlayerIntentComponent.h"
#include "Game/ECS/Systems/PlayerMovementUtils.h"

namespace
{
	// ƒ{ƒ^ƒ“1‚Â‚Ô‚ñ‚ÌˆÓ}‚ğXV‚·‚é
	void UpdateAction(ActionIntent& action, bool pressed, bool held, float bufferTime, float elapsedTime)
	{
		// ‰Ÿ‚µ‘±‚¯‚Ä‚¢‚éŠÔ
		action.held = held;
		action.heldTime = held ? action.heldTime + elapsedTime : 0.0f;

		// æs“ü—ÍF‰Ÿ‚³‚ê‚½uŠÔ‚Éó‚¯•t‚¯Aó•tŠÔ‚ğ‰ß‚¬‚½‚çæ‚èÁ‚·
		if (pressed)
		{
			action.buffered = true;
			action.bufferTimer = 0.0f;
		}
		else if (action.buffered)
		{
			action.bufferTimer += elapsedTime;
			if (action.bufferTimer > bufferTime)
			{
				action.buffered = false;
			}
		}
	}
}

void PlayerIntentSystem::Update(entt::registry& registry, float elapsedTime, const SceneContext& context)
{
	// ƒJƒƒ‰‚ª‚È‚¢ê‡‚Íˆ—‚µ‚È‚¢
	if (!context.camera)return;

	// ƒJƒƒ‰‚Ì‘O•ûŒü‚Æ‰E•ûŒü‚ğæ“¾
	const DirectX::XMFLOAT3 front = context.camera->GetFront();
	const DirectX::XMFLOAT3 right = context.camera->GetRight();

	auto view = registry.view<GameInputComponent, PlayerIntentComponent>();
	view.each([&](const GameInputComponent& input, PlayerIntentComponent& intent)
		{
			// ¶‚Ì“ü—Í‚ğAƒJƒƒ‰Šî€‚Ìƒ[ƒ‹ƒh•ûŒü‚É•ÏŠ·
			auto dir = PlayerMovementUtils::StickToWorldVelocity(input.moveStick.x, input.moveStick.y, front, right, 1.0f);
			intent.hasRawInput = (dir.inputLength > 0.0f);

			// “®‚«‚½‚¢‚©(—P—\•t‚«)
			if (intent.hasRawInput)
			{
				// “ü—Í‚ ‚èF•ûŒü‚Æ“|‚µ‹ï‡‚ğXV‚µ‚ÄA—P—\‚ğƒŠƒZƒbƒg
				intent.moveDir = dir.velocity; // ’·‚³1
				intent.moveAmount = (std::min)(dir.inputLength, 1.0f);
				intent.graceTimer = 0.0f;
				intent.wantsMove = true;
			}
			else
			{
				// “ü—Í‚È‚µF—P—\‚ÌŠÔ‚ÍAÅŒã‚Ì•ûŒü‚Æ“|‚µ‹ï‡‚ğ•Û‚Â
				intent.graceTimer += elapsedTime;
				if (intent.graceTimer >= intent.moveGraceTime)
				{
					intent.wantsMove = false;
					intent.moveAmount = 0.0f;
				}
			}

			// ‘–‚è‚½‚¢‚©(ƒqƒXƒeƒŠƒVƒX{—P—\)
			if (!intent.wantsMove)
			{
				// “®‚«‚½‚­‚È‚¢F—P—\‚È‚µ‚Å~‚ß‚é
				intent.wantsRun = false;
				intent.runGraceTimer = 0.0f;
			}
			else if (intent.moveAmount >= intent.runEnterThreshold)
			{
				// [‚­“|‚µ‚½F‘–‚éB—P—\‚ğ‘ª‚è’¼‚·
				intent.wantsRun = true;
				intent.runGraceTimer = 0.0f;
			}
			else if (intent.moveAmount < intent.runExitThreshold)
			{
				// ó‚­‚È‚Á‚½F—P—\‚ÌŠÔ‚¾‚¯‘Ò‚Á‚Ä‚©‚ç•à‚«‚É–ß‚·
				intent.runGraceTimer += elapsedTime;
				if (intent.runGraceTimer >= intent.runGraceTime)
				{
					intent.wantsRun = false;
				}
			}
			else
			{
				// ‚»‚ÌŠÔF¡‚Ìó‘Ô‚ğ•Û‚Â
				intent.runGraceTimer = 0.0f;
			}

			// ƒ{ƒ^ƒ“‚ÌˆÓ}
			UpdateAction(intent.jump, input.jump.pressed, input.jump.held, intent.jumpBufferTime, elapsedTime);
			UpdateAction(intent.dodge, input.dodge.pressed, input.dodge.held, intent.dodgeBufferTime, elapsedTime);
		});
}