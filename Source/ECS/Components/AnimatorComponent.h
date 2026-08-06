#pragma once
#include <string>
#include <DirectXMath.h>

struct AnimatorComponent
{
    std::string currentAnimation;      // 現在のアニメーション名
    float       animationTime{ 0.0f }; // 現在の再生時間
    float       playSpeed{ 1.0f };     // 再生速度
    bool        loop{ true };          // ループフラグ
    bool        isPlaying{ true };     // 再生中フラグ
    bool        isEnd{ false };        // 再生終了フラグ

    // ブレンド用
    std::string blendAnimation;      // ブレンド先アニメーション名
    float       blendTime{ 0.0f };   // ブレンド時間
    float       blendRate{ 0.0f };   // ブレンド率（0.0～1.0）
    bool        isBlending{ false }; // ブレンド中フラグ

    // ルートモーション用
    bool        useRootMotion { false }; // ルートモーション使用
    bool        useRootMotionY{ false }; // Y軸適応
    std::string rootMotionNodeName;      // ルートノード名
    DirectX::XMFLOAT3 rootMotionDelta{ 0.0f, 0.0f, 0.0f }; // ルートモーション移動量


    // State遷移用
	std::string animatorGraphPath; // .animatorファイルのパス
	int         currentParentStateId{ -1 }; // 現在の親ステートID
	int         currentChildStateId{ -1 };  // 現在の子ステートID
};