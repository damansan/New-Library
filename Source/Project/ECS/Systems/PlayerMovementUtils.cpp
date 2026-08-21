#include "PlayerMovementUtils.h"
#include <cmath>

namespace PlayerMovementUtils
{
	WorldVelocityResult StickToWorldVelocity(
		float inputX, float inputZ,
		const DirectX::XMFLOAT3& cameraFront,
		const DirectX::XMFLOAT3& cameraRight,
		float speed)
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
			result.velocity.x = (dirX / len) * speed;
			result.velocity.z = (dirZ / len) * speed;
		}

		return result;
	}

	float CalcFacingYaw(float worldDirX, float worldDirZ)
	{
		return atan2f(-worldDirX, -worldDirZ);
	}
}