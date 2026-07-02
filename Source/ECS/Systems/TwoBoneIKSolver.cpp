#include "TwoBoneIKSolver.h"
#include <algorithm>
#include <cmath>

namespace TwoBoneIKSolver
{
    namespace
    {
        using namespace DirectX;

        // 安全なacos(浮動小数の誤差でxが[-1,1]をわずかに超えることがあるため)
        float SafeAcos(float x)
        {
            return std::acos(std::clamp(x, -1.0f, 1.0f));
        }

        // 3辺の長さ(a, b, c)から成る三角形の、辺a・辺bに挟まれた角(=辺cの対角)を求める
        // 余弦定理: c^2 = a^2 + b^2 - 2ab*cos(C)  →  C = acos((a^2+b^2-c^2) / (2ab))
        float TriangleAngle(float a, float b, float oppositeC)
        {
            float denom = 2.0f * a * b;
            if (denom < 1e-6f)
            {
                // ボーン長がほぼ0のような異常値の保険
                return 0.0f;
            }
            float cosAngle = (a * a + b * b - oppositeC * oppositeC) / denom;
            return SafeAcos(cosAngle);
        }

        // root-target間の距離を、実際に届く範囲にクランプする
        float ClampTargetLength(float rawLength, float upperLen, float lowerLen)
        {
            // 伸びきった状態(ロック)を防ぐため、わずかに内側に余裕を持たせる
            constexpr float epsilon = 0.001f;
            float maxLength = upperLen + lowerLen - epsilon;
            float minLength = std::abs(upperLen - lowerLen) + epsilon;
            return std::clamp(rawLength, minLength, maxLength);
        }

        // root→target方向を軸に、poleVectorが乗っている「曲げ平面」の法線を求める
        // この法線を軸にmid関節を回転させることで、膝が狙った方向に曲がる
        XMVECTOR CalcBendAxis(
            FXMVECTOR rootToTarget, // 正規化済みでなくてよい
            FXMVECTOR rootToPole)   // 正規化済みでなくてよい
        {
            XMVECTOR axis = XMVector3Cross(rootToTarget, rootToPole);
            float lengthSq = XMVectorGetX(XMVector3LengthSq(axis));
            if (lengthSq < 1e-8f)
            {
                // rootToTargetとrootToPoleがほぼ平行(poleVectorが役に立たない位置にある)
                // 保険として適当な垂直軸を作る
                XMVECTOR fallback = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
                axis = XMVector3Cross(rootToTarget, fallback);
                if (XMVectorGetX(XMVector3LengthSq(axis)) < 1e-8f)
                {
                    fallback = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
                    axis = XMVector3Cross(rootToTarget, fallback);
                }
            }
            return XMVector3Normalize(axis);
        }

        // 現在のdirectionを、axis周りにangle分だけ回転させたベクトルを返す
        XMVECTOR RotateAroundAxis(FXMVECTOR direction, FXMVECTOR axis, float angle)
        {
            XMVECTOR rotationQuat = XMQuaternionRotationAxis(axis, angle);
            return XMVector3Rotate(direction, rotationQuat);
        }

        // fromベクトルをtoベクトルに向ける、最短回転のクォータニオンを求める
        XMVECTOR RotationFromToRotation(FXMVECTOR from, FXMVECTOR to)
        {
            XMVECTOR fromN = XMVector3Normalize(from);
            XMVECTOR toN = XMVector3Normalize(to);

            float dot = std::clamp(XMVectorGetX(XMVector3Dot(fromN, toN)), -1.0f, 1.0f);

            if (dot > 0.9999f)
            {
                // ほぼ同じ方向: 回転不要
                return XMQuaternionIdentity();
            }
            if (dot < -0.9999f)
            {
                // ほぼ正反対: 任意の垂直軸で180度回転
                XMVECTOR fallback = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
                XMVECTOR axis = XMVector3Cross(fromN, fallback);
                if (XMVectorGetX(XMVector3LengthSq(axis)) < 1e-8f)
                {
                    fallback = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
                    axis = XMVector3Cross(fromN, fallback);
                }
                return XMQuaternionRotationAxis(XMVector3Normalize(axis), XM_PI);
            }

            XMVECTOR axis = XMVector3Normalize(XMVector3Cross(fromN, toN));
            float angle = SafeAcos(dot);
            return XMQuaternionRotationAxis(axis, angle);
        }
    }

    Result Solve(
        const XMFLOAT3& root,
        const XMFLOAT3& mid,
        const XMFLOAT3& end,
        const XMFLOAT4& rootRotation,
        const XMFLOAT4& midRotation,
        const XMFLOAT3& target,
        const XMFLOAT3& poleVector)
    {
        using namespace DirectX;

        XMVECTOR rootV = XMLoadFloat3(&root);
        XMVECTOR midV = XMLoadFloat3(&mid);
        XMVECTOR endV = XMLoadFloat3(&end);
        XMVECTOR targetV = XMLoadFloat3(&target);
        XMVECTOR poleV = XMLoadFloat3(&poleVector);

        // 各辺のベクトルと長さ
        XMVECTOR ab = midV - rootV;     // root→mid (上の骨)
        XMVECTOR bc = endV - midV;      // mid→end (下の骨)
        XMVECTOR ac = endV - rootV;     // root→end (現在の姿勢での対角)
        XMVECTOR at = targetV - rootV;  // root→target

        float upperLen = XMVectorGetX(XMVector3Length(ab));
        float lowerLen = XMVectorGetX(XMVector3Length(bc));
        float acLen = XMVectorGetX(XMVector3Length(ac));
        float rawTargetLen = XMVectorGetX(XMVector3Length(at));

        float targetLen = ClampTargetLength(rawTargetLen, upperLen, lowerLen);

        // 曲げ平面の軸(root→target と root→poleVector が乗る平面の法線)
        XMVECTOR bendAxis = CalcBendAxis(at, poleV - rootV);

        // mid関節の角度: 現在の角度 → targetLenに対応する角度、への差分を求める
        float oldMidAngle = TriangleAngle(upperLen, lowerLen, acLen);
        float newMidAngle = TriangleAngle(upperLen, lowerLen, targetLen);
        float deltaMidAngle = newMidAngle - oldMidAngle;

        // mid関節をbendAxis周りにdeltaMidAngleだけ回転させる
        XMVECTOR midDeltaRotation = XMQuaternionRotationAxis(bendAxis, deltaMidAngle);
        XMVECTOR midRotationV = XMLoadFloat4(&midRotation);
        XMVECTOR bentMidRotationV = XMQuaternionMultiply(midRotationV, midDeltaRotation);

        // mid関節を回したことで、bc(mid→end)の向きも一緒に変わる
        XMVECTOR newBc = RotateAroundAxis(bc, bendAxis, deltaMidAngle);
        XMVECTOR newAc = ab + newBc; // root→新しいend の方向・距離(長さ = targetLen になっているはず)

        // newAc(曲げた後のroot→end)を、at(root→target)の方向に向ける回転を求める
        XMVECTOR aimRotation = RotationFromToRotation(newAc, at);

        // root回転にaimRotationを上乗せする
        XMVECTOR rootRotationV = XMLoadFloat4(&rootRotation);
        XMVECTOR newRootRotationV = XMQuaternionMultiply(rootRotationV, aimRotation);

        // rootが回ればmid, endも一緒に回るので、midの回転にも同じ分を上乗せする
        XMVECTOR finalMidRotationV = XMQuaternionMultiply(bentMidRotationV, aimRotation);

        // 最終的なmid, endのワールド位置を求める
        XMVECTOR finalAb = XMVector3Rotate(ab, aimRotation);
        XMVECTOR finalMidPositionV = rootV + finalAb;

        XMVECTOR finalBc = XMVector3Rotate(newBc, aimRotation);
        XMVECTOR finalEndPositionV = finalMidPositionV + finalBc;

        Result result{};
        XMStoreFloat3(&result.midPosition, finalMidPositionV);
        XMStoreFloat4(&result.rootRotation, newRootRotationV);
        XMStoreFloat4(&result.midRotation, finalMidRotationV);
        XMStoreFloat3(&result.endPosition, finalEndPositionV);
        return result;
    }
}