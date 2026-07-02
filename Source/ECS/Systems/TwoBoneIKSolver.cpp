#include "TwoBoneIKSolver.h"
#include <algorithm>
#include <cmath>

using namespace DirectX;

namespace
{
	constexpr float Epsilon = 0.0001f;

	// 2ベクトル間の角度を求める
	float AngleBetween(FXMVECTOR v1, FXMVECTOR v2)
	{
		XMVECTOR n1 = XMVector3Normalize(v1);
		XMVECTOR n2 = XMVector3Normalize(v2);
		float dot = std::clamp(XMVectorGetX(XMVector3Dot(n1, n2)), -1.0f, 1.0f);
		return acosf(dot);
	}
}

namespace TwoBoneIKSolver
{
	Result Solve(
		const DirectX::XMFLOAT3& rootPosIn,
		const DirectX::XMFLOAT3& midPosIn,
		const DirectX::XMFLOAT3& endPosIn,
		const DirectX::XMFLOAT3& targetPosIn,
		const DirectX::XMFLOAT3& poleVectorIn,
		const DirectX::XMFLOAT4& rootGlobalRotationIn)
	{
		XMVECTOR a = XMLoadFloat3(&rootPosIn);
		XMVECTOR b = XMLoadFloat3(&midPosIn);
		XMVECTOR c = XMLoadFloat3(&endPosIn);
		XMVECTOR t = XMLoadFloat3(&targetPosIn);
		XMVECTOR pole = XMLoadFloat3(&poleVectorIn);
		XMVECTOR rootRot = XMLoadFloat4(&rootGlobalRotationIn);

		// 骨の長さ
		float lenAB = XMVectorGetX(XMVector3Length(b - a));
		float lenBC = XMVectorGetX(XMVector3Length(c - b));

		// targetまでの距離を腕(脚)が届く範囲にクランプする
		float lenAT = XMVectorGetX(XMVector3Length(t - a));
		lenAT = std::clamp(lenAT, Epsilon, lenAB + lenBC - Epsilon);

		// 現在の内角（root側）
		float currentRootAngle = AngleBetween(b - a, c - a);
		// 現在の内角（mid側）
		float currentMidAngle = AngleBetween(a - b, c - b);

		// targetに届いたときのroot側の内角
		float targetRootAngle = acosf(std::clamp(
			(lenBC * lenBC - lenAB * lenAB - lenAT * lenAT) / (-2.0f * lenAB * lenAT),
			-1.0f, 1.0f));
		// targetに届いたときのmid側の内角
		float targetMidAngle = acosf(std::clamp(
			(lenAT * lenAT - lenAB * lenAB - lenBC * lenBC) / (-2.0f * lenAB * lenBC),
			-1.0f, 1.0f));

		// 現在の姿勢
		XMVECTOR bendAxis = XMVector3Normalize(XMVector3Cross(c - a, b - a));

		// 3点がほぼ一直線で軸が定まらない場合は、poleVectorから軸を作って代用する
		if (XMVectorGetX(XMVector3LengthSq(bendAxis)) < Epsilon)
		{
			bendAxis = XMVector3Normalize(XMVector3Cross(pole - a, b - a));
		}

		float deltaRootAngle = targetRootAngle - currentRootAngle;
		float deltaMidAngle = targetMidAngle - currentMidAngle;

		XMVECTOR qBendRoot = XMQuaternionRotationAxis(bendAxis, deltaRootAngle);
		XMVECTOR qBendMid = XMQuaternionRotationAxis(bendAxis, deltaMidAngle);

		// 曲げただけの状態でのmid/end位置を仮計算
		XMVECTOR midBent = a + XMVector3Rotate(b - a, qBendRoot);
		XMVECTOR endBent = midBent + XMVector3Rotate(c - b, XMQuaternionMultiply(qBendRoot, qBendMid));

		// 全体をtarget方向へ直す
		XMVECTOR currentEndDir = XMVector3Normalize(endBent - a);
		XMVECTOR desiredEndDir = XMVector3Normalize(t - a);

		XMVECTOR aimAxis = XMVector3Normalize(XMVector3Cross(currentEndDir, desiredEndDir));
		XMVECTOR qAim = XMQuaternionIdentity();
		if (XMVectorGetX(XMVector3LengthSq(aimAxis)) > Epsilon)
		{
			float aimAngle = AngleBetween(currentEndDir, desiredEndDir);
			qAim = XMQuaternionRotationAxis(aimAxis, aimAngle);
		}

		// target方向に軸をひねり合わせる
		XMVECTOR midAimed = a + XMVector3Rotate(midBent - a, qAim);
		XMVECTOR twistAxis = desiredEndDir; // target方向を軸にひねる

		// 現在の曲がってる向き
		XMVECTOR currentBendDir = midAimed - a;
		currentBendDir = currentBendDir - twistAxis * XMVector3Dot(currentBendDir, twistAxis);

		// 曲がってほしい向き
		XMVECTOR desiredBendDir = pole - a;
		desiredBendDir = desiredBendDir - twistAxis * XMVector3Dot(desiredBendDir, twistAxis);

		XMVECTOR qTwist = XMQuaternionIdentity();
		if (XMVectorGetX(XMVector3LengthSq(currentBendDir)) > Epsilon &&
			XMVectorGetX(XMVector3LengthSq(desiredBendDir)) > Epsilon)
		{
			currentBendDir = XMVector3Normalize(currentBendDir);
			desiredBendDir = XMVector3Normalize(desiredBendDir);
			
			float twistAngle = AngleBetween(currentBendDir, desiredBendDir);

			// 回転の向きを判定する
			XMVECTOR cross = XMVector3Cross(currentBendDir, desiredBendDir);
			if (XMVectorGetX(XMVector3Dot(cross, twistAxis)) < 0.0f)
			{
				twistAngle = -twistAngle;
			}
			qTwist = XMQuaternionRotationAxis(twistAxis, twistAngle);
		}

		// root：曲げ→向き直し→ひねり、の順に合成した最終グローバル回転
		XMVECTOR rootRotFinal = XMQuaternionMultiply(
			XMQuaternionMultiply(XMQuaternionMultiply(rootRot, qBendRoot), qAim), qTwist);

		Result result;
		XMStoreFloat4(&result.rootGlobalRotation, rootRotFinal);
		XMStoreFloat3(&result.midBendAxis, bendAxis);
		result.midBendAngleDelta = deltaMidAngle;
		return result;
	}
}