#include "PlayerWallRunSystem.h"
#include <cmath>
#include <algorithm>

#include "Game/ECS/Components/PlayerWallRunComponent.h"
#include "Game/ECS/Components/PlayerWallSenseComponent.h"
#include "Game/ECS/Components/PlayerLocomotionComponent.h"
#include "Game/ECS/Systems/PlayerMovementUtils.h"

#include "Engine/ECS/Components/CharacterContactComponent.h"
#include "Engine/ECS/Components/MovementComponent.h"
#include "Engine/ECS/Components/TransformComponent.h"
#include "Engine/ECS/Components/StateMachineComponent.h"
#include "Engine/ECS/Systems/StateMachineSystem.h"

namespace
{
	// 水平の向きを長さ1にする
	bool NormalizeXZ(float& x, float& z)
	{
		const float len = sqrtf(x * x + z * z);
		if (len < 1e-4f) return false;
		x /= len;
		z /= len;
		return true;
	}

	// 向き(dx, dz)から壁へ向かう成分を取り除き、壁の面に沿わせる
	bool ProjectOnWall(float& dx, float& dz, float nx, float nz)
	{
		const float d = dx * nx + dz * nz;
		dx -= d * nx;
		dz -= d * nz;
		return NormalizeXZ(dx, dz);
	}
}

PlayerWallRunSystem::PlayerWallRunSystem()
{
	StateMachineSystem::RequireState<PlayerWallRunComponent>("WallRun", "PlayerWallRunSystem");
}

void PlayerWallRunSystem::Update(entt::registry& registry, float elapsedTime, const SceneContext& context)
{
	(void)context;

	auto view = registry.view<PlayerWallRunComponent, PlayerWallSenseComponent, CharacterContactComponent,
		StateMachineComponent, PlayerLocomotionComponent, MovementComponent, TransformComponent>();

	view.each([&](PlayerWallRunComponent& wallRun, const PlayerWallSenseComponent& sense, const CharacterContactComponent& contact,
		const StateMachineComponent& stateMachine, PlayerLocomotionComponent& loco, MovementComponent& movement, const TransformComponent& transform)
		{
			if (!stateMachine.IsIn("WallRun")) return;

			// 毎フレーム：壁に当たっていれば、法線を取り直す
			float nx = contact.sideNormal.x;
			float nz = contact.sideNormal.z;
			if (contact.hitSide && NormalizeXZ(nx, nz))
			{
				wallRun.wallNormal = { nx, 0.0f, nz };
			}
			const float wnx = wallRun.wallNormal.x;
			const float wnz = wallRun.wallNormal.z;

			if (stateMachine.enteredThisFrame)
			{
				// 進む向き：今の速度を壁の面に沿わせる
				float dx = movement.velocity.x;
				float dz = movement.velocity.z;
				if (!ProjectOnWall(dx, dz, wnx, wnz))
				{
					// 速度から決まらない：体の向きを使う
					const DirectX::XMFLOAT3 forward = PlayerMovementUtils::YawToForward(transform.rotation.y);
					dx = forward.x;
					dz = forward.z;
					ProjectOnWall(dx, dz, wnx, wnz);
				}
				wallRun.runDir = { dx, 0.0f, dz };

				// 走る速さ：入った勢いを保つ
				const float planarSpeed = sqrtf(movement.velocity.x * movement.velocity.x + movement.velocity.z * movement.velocity.z);
				wallRun.runSpeed = (std::max)(planarSpeed, wallRun.minRunSpeed);

				// クリップの選択用：入った瞬間だけ決める
				wallRun.isWallRight = sense.isWallRight;
				wallRun.elapsed = 0.0f;

				// 1回限りの縦の速さは、ステートが直接書く
				movement.velocity.y = (std::clamp)(movement.velocity.y, wallRun.minEnterVerticalSpeed, wallRun.maxEnterVerticalSpeed);
			}
			else
			{
				// 走っている向きを、今の壁の面に沿わせ直す
				float dx = wallRun.runDir.x;
				float dz = wallRun.runDir.z;
				if (ProjectOnWall(dx, dz, wnx, wnz))
				{
					wallRun.runDir = { dx, 0.0f, dz };
				}
			}

			wallRun.elapsed += elapsedTime;

			// 毎フレーム：Locomotion への指示
			float mx = wallRun.runDir.x * wallRun.runSpeed - wnx * wallRun.stickSpeed;
			float mz = wallRun.runDir.z * wallRun.runSpeed - wnz * wallRun.stickSpeed;
			const float targetSpeed = sqrtf(mx * mx + mz * mz);
			NormalizeXZ(mx, mz);

			LocomotionCommand& cmd = loco.command;
			cmd.mode = LocomotionMode::Directional;
			cmd.moveDir = { mx, 0.0f, mz };
			cmd.targetSpeed = targetSpeed;
			cmd.accelHalfLife = wallRun.accelHalfLife;
			cmd.faceMoveDir = true;

			// 毎フレーム：Movement への指示
			movement.gravityScale = wallRun.gravityScale;
		});
}