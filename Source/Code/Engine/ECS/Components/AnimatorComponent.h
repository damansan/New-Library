#pragma once
#include <string>
#include <DirectXMath.h>
#include "Engine/ECS/Reflect.h"

struct AnimatorComponent
{
    REFLECT_BEGIN(AnimatorComponent,"Animator")

        // 再生関連
        REFLECT_FIELD(float, playSpeed, (1.0f),  ("Play Speed", 0.0f, 3.0f))                 // 再生速度
        REFLECT_FIELD(bool,  loop,      (true),  ("Loop"))                                   // ループフラグ
        REFLECT_FIELD(bool,  isEnd,     (false), ("Is End",     0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 再生終了フラグ

        // ブレンド関連
        REFLECT_FIELD(float, blendRate, (0.0f),  ("Blend Rate",  0.0f, 1.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // ブレンドの進み具合
        REFLECT_FIELD(bool, isBlending, (false), ("Is Blending", 0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // ブレンド中か

        // ルートモーション関連
        REFLECT_FIELD(bool, useRootMotionY,            (false), ("Use Root Motion Y")) // Y軸適応
        REFLECT_FIELD(bool, useRootMotion,             (false), ("Use Root Motion"))   // ルートモーション使用
        REFLECT_FIELD(std::string, rootMotionNodeName, (""),    ("Root Motion Node"))  // ルートノード名
      
    REFLECT_END()


    // 再生関連
    std::string currentAnimation;      // 現在のアニメーション名
    float       animationTime{ 0.0f }; // 現在の再生時間
    bool        isPlaying{ true };     // 再生中フラグ

    // ブレンド関連
    float blendTime{ 0.0f };              // ブレンドにかける時間
    bool  blendSnapshotPending{ false };  // 次の更新で、前のフレームの最終ポーズを保存するか
    
    // 混ぜる元のポーズ
    std::vector<DirectX::XMFLOAT3> blendFromPositions;
    std::vector<DirectX::XMFLOAT4> blendFromRotations;
    std::vector<DirectX::XMFLOAT3> blendFromScales;

    // ルートモーション関連
    DirectX::XMFLOAT3 rootMotionDelta{ 0.0f, 0.0f, 0.0f }; // ルートモーション移動量

    // アニメーショングラフ
	int currentParentStateId{ -1 }; // 現在の親ステートID
	int currentChildStateId{ -1 };  // 現在の子ステートID

    // トリミング範囲
    float trimStart{ 0.0f };
    float trimEnd{ -1.0f };
    bool  trimResolved{ false };

    // 速度カーブ
    float speedCurveMultiplier{ 1.0f };
};