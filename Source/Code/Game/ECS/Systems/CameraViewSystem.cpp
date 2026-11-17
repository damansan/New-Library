#include "CameraViewSystem.h"
#include <cmath>

#include "Game/ECS/Components/CameraViewComponent.h"
#include "Game/ECS/Components/CameraEffectComponent.h"
#include "Game/ECS/Components/GameCameraComponent.h"

void CameraViewSystem::Update(entt::registry& registry)
{
	auto view = registry.view<CameraViewComponent, GameCameraComponent>();
	view.each([&](entt::entity entity, CameraViewComponent& camView, const GameCameraComponent& cam)
		{
			// “y‘ä‚ÌˆÊ’u‡‚í‚¹‚ª‚Ü‚¾‚È‚çAì‚ç‚È‚¢
			camView.valid = false;
			if (!cam.initialized) return;

			// “y‘ä
			camView.eye = cam.base.currentEye;
			camView.target = cam.base.currentTarget;
			camView.fov = cam.fov;
			camView.roll = 0.0f;

			// { ‰‰o
			const auto* effect = registry.try_get<CameraEffectComponent>(entity);
			if (effect)
			{
				camView.eye.x += effect->eyeOffset.x;
				camView.eye.y += effect->eyeOffset.y;
				camView.eye.z += effect->eyeOffset.z;
				camView.target.x += effect->targetOffset.x;
				camView.target.y += effect->targetOffset.y;
				camView.target.z += effect->targetOffset.z;
				camView.fov += effect->fovOffset;
				camView.roll += effect->roll;
			}

			// ‚ ‚½‚è”»’è

			camView.valid = true;
		});
}

// Camera ‚É“n‚·
void CameraViewSystem::ApplyToCamera(entt::registry& registry, Camera& camera)
{
	auto view = registry.view<CameraViewComponent>();
	for (auto entity : view)
	{
		const auto& camView = view.get<CameraViewComponent>(entity);
		if (!camView.valid) continue;

		// ŒX‚«FŒ©‚éŒü‚«‚ğ²‚ÉAã‚ÌŒü‚«(0,1,0)‚ğ roll ‚¾‚¯‰ñ‚·
		const DirectX::XMVECTOR eye = DirectX::XMLoadFloat3(&camView.eye);
		const DirectX::XMVECTOR target = DirectX::XMLoadFloat3(&camView.target);
		const DirectX::XMVECTOR forward = DirectX::XMVector3Normalize(DirectX::XMVectorSubtract(target, eye));
		const DirectX::XMMATRIX rollMatrix = DirectX::XMMatrixRotationAxis(forward, DirectX::XMConvertToRadians(camView.roll));
		DirectX::XMFLOAT3 up;
		DirectX::XMStoreFloat3(&up, DirectX::XMVector3TransformNormal(DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), rollMatrix));

		camera.SetLookAt(camView.eye, camView.target, up);
		camera.SetFovDeg(camView.fov);
		break; // ƒJƒƒ‰‚Í1‚Â‚¾‚¯
	}
}
