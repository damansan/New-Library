#pragma once
#include <DirectXMath.h>

// プレイヤーの移動計算
namespace PlayerMovementUtils
{
	// スティック入力とカメラの前方/右方向から、ワールド座標系の速度ベクトルと、
	// スティックの倒し具合を求める
	struct WorldVelocityResult
	{
		DirectX::XMFLOAT3 velocity{ 0.0f, 0.0f, 0.0f };
		float inputLength = 0.0f; // スティックの倒し具合
	};

	WorldVelocityResult StickToWorldVelocity(
		float inputX, float inputZ,
		const DirectX::XMFLOAT3& cameraFront,
		const DirectX::XMFLOAT3& cameraRight,
		float speed);

	// ワールド座標系の移動方向から、Y軸回転角を求める
	float CalcFacingYaw(float worldDirX, float worldDirZ);

}