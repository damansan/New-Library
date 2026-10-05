#include "PlayerMovementUtils.h"
#include <cmath>

namespace PlayerMovementUtils
{
	WorldVelocityResult StickToWorldVelocity(
		float inputX, float inputZ,
		const DirectX::XMFLOAT3& cameraFront,
		const DirectX::XMFLOAT3& cameraRight,
		float maxSpeed)
	{
		WorldVelocityResult result;

		float frontLen = sqrtf(cameraFront.x * cameraFront.x + cameraFront.z * cameraFront.z);
		float rightLen = sqrtf(cameraRight.x * cameraRight.x + cameraRight.z * cameraRight.z);

		float frontX = (frontLen > 0.0f) ? cameraFront.x / frontLen : 0.0f;
		float frontZ = (frontLen > 0.0f) ? cameraFront.z / frontLen : 0.0f;
		float rightX = (rightLen > 0.0f) ? cameraRight.x / rightLen : 0.0f;
		float rightZ = (rightLen > 0.0f) ? cameraRight.z / rightLen : 0.0f;

		float dirX = frontX * inputZ + rightX * inputX;
		float dirZ = frontZ * inputZ + rightZ * inputX;
		float len = sqrtf(dirX * dirX + dirZ * dirZ);

		result.inputLength = len;
		
		if (len > 0.0f)
		{
			result.velocity.x = (dirX / len) * maxSpeed;
			result.velocity.z = (dirZ / len) * maxSpeed;
		}

		return result;
	}

	WorldVelocityResult ResolvePlanarVelocity(
		float inputX, float inputZ, 
		const DirectX::XMFLOAT3& cameraFront, 
		const DirectX::XMFLOAT3& cameraRight,
		float targetSpeed, 
		const DirectX::XMFLOAT3& currentVelocity, 
		float elapsedTime)
	{
		(void)currentVelocity;
		(void)elapsedTime;

		return StickToWorldVelocity(inputX, inputZ, cameraFront, cameraRight, targetSpeed);
	}

	float CalcFacingYaw(float worldDirX, float worldDirZ)
	{
		return atan2f(-worldDirX, -worldDirZ);
	}
	float RotateTowards(float currentYaw, float targetYaw, float turnSpeed, float elapsedTime)
	{
		float diff = targetYaw - currentYaw;
		while (diff > DirectX::XM_PI)  diff -= DirectX::XM_2PI;
		while (diff < -DirectX::XM_PI) diff += DirectX::XM_2PI;

		float maxStep = turnSpeed * elapsedTime;
		if (diff > maxStep)       diff = maxStep;
		else if (diff < -maxStep) diff = -maxStep;

		float result = currentYaw + diff;

		while (result > DirectX::XM_PI)  result -= DirectX::XM_2PI;
		while (result < -DirectX::XM_PI) result += DirectX::XM_2PI;

		return result;
	}
}