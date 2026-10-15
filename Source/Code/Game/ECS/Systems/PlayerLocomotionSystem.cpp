#include "PlayerLocomotionSystem.h"
#include <cmath>
#include <algorithm>
#include <DirectXMath.h>

#include "Game/ECS/Components/PlayerLocomotionComponent.h"
#include "Game/ECS/Systems/PlayerMovementUtils.h"
#include "Engine/ECS/Components/MovementComponent.h"
#include "Engine/ECS/Components/TransformComponent.h"

namespace
{
	// 指数的な減衰は0に届かないので、目標が0の時はこれより遅くなったら止める
	constexpr float kStopSpeed = 0.05f;

	float PlanarLength(float x, float z) { return sqrtf(x * x + z * z); }

	// Locomotion が速さと向きを動かす
	void Drive(PlayerLocomotionComponent& loco, const LocomotionCommand& cmd,
		MovementComponent& movement, TransformComponent& transform, float elapsedTime)
	{
		// 1) 向き：指示された方向へ回す
		float facingError = 0.0f;
		if (PlanarLength(cmd.moveDir.x, cmd.moveDir.z) > 0.0001f)
		{
			float targetYaw = PlayerMovementUtils::CalcFacingYaw(cmd.moveDir.x, cmd.moveDir.z);
			transform.rotation.y = PlayerMovementUtils::RotateTowards(transform.rotation.y, targetYaw, cmd.turnSpeed, elapsedTime);

			// 回った後の向きで、ずれを測る
			facingError = PlayerMovementUtils::AngleDiff(transform.rotation.y, targetYaw);
		}

		// 2) 速さ：ずれが大きいほど目標を下げ(90°以上で0)、ダンパーで近づける
		float alignment = (std::max)(0.0f, cosf(facingError));
		float desiredSpeed = cmd.targetSpeed * alignment;

		float halfLife = (desiredSpeed > loco.currentSpeed) ? cmd.accelHalfLife : cmd.decelHalfLife;
		loco.currentSpeed = PlayerMovementUtils::Damper(loco.currentSpeed, desiredSpeed, halfLife, elapsedTime);

		if (desiredSpeed <= 0.0f && loco.currentSpeed < kStopSpeed)
		{
			loco.currentSpeed = 0.0f;
		}

		// 3) 速度：体の向きに沿って書く
		DirectX::XMFLOAT3 forward = PlayerMovementUtils::YawToForward(transform.rotation.y);
		movement.velocity.x = forward.x * loco.currentSpeed;
		movement.velocity.z = forward.z * loco.currentSpeed;

		// 確認用
		loco.desiredSpeed = desiredSpeed;
		loco.facingErrorDeg = DirectX::XMConvertToDegrees(facingError);
	}
}


void PlayerLocomotionSystem::Update(entt::registry& registry, float elapsedTime)
{
	auto view = registry.view<PlayerLocomotionComponent, MovementComponent, TransformComponent>();
	view.each([&](PlayerLocomotionComponent& loco, MovementComponent& movement, TransformComponent& transform)
		{
			const LocomotionCommand& cmd = loco.command;

			switch (cmd.mode)
			{
			case LocomotionMode::Drive:
				Drive(loco, cmd, movement, transform, elapsedTime);
				break;

			case LocomotionMode::External:
				// velocity は外部が書く。Drive に戻った時につながるよう、実際の速さを写しておく
				loco.currentSpeed = PlanarLength(movement.velocity.x, movement.velocity.z);
				loco.desiredSpeed = loco.currentSpeed;
				loco.facingErrorDeg = 0.0f;
				break;
			}

			// 指示は1フレーム限り。書き忘れたら止まるように、初期値に戻す
			loco.command = LocomotionCommand{};
		});
}