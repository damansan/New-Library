#pragma once
#include <string>
#include <vector>
#include <DirectXMath.h>
#include "Engine/ECS/Reflect.h"

// 今再生しているものの種類
enum class AnimPlayMode
{
    Clip,  // クリップを1本
    Blend, // Blend ノード
};

struct AnimatorComponent
{
    REFLECT_BEGIN(AnimatorComponent, "Animator")

        // 再生
        REFLECT_FIELD(float, playSpeed, (1.0f),  ("Play Speed", 0.0f, 3.0f)) // 再生速度
        REFLECT_FIELD(bool,  loop,      (true),  ("Loop"))                   // ループするか
        REFLECT_FIELD(bool,  isEnd,     (false), ("Is End", 0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 最後まで再生したか

        // クロスフェード
        REFLECT_FIELD(float, crossfadeRate, (0.0f), ("Crossfade Rate", 0.0f, 1.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 進み具合(0〜1)
        REFLECT_FIELD(bool, isCrossfading, (false), ("Is Crossfading", 0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // クロスフェード中か

        // Blend ノード
        REFLECT_FIELD(std::string, blendClipA, (""),   ("Blend Clip A", 0.0f, 0.0f, 0.1f, "",  FieldFlags::VisibleOnly | FieldFlags::Transient)) // 材料Aのクリップ名
        REFLECT_FIELD(std::string, blendClipB, (""),   ("Blend Clip B", 0.0f, 0.0f, 0.1f, "",  FieldFlags::VisibleOnly | FieldFlags::Transient)) // 材料Bのクリップ名(材料が1つなら空)
        REFLECT_FIELD(float,       blendWeight,(0.0f), ("Blend Weight", 0.0f, 1.0f, 0.01f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // Bの割合(0〜1)
        REFLECT_FIELD(float,       blendPhase, (0.0f), ("Blend Phase",  0.0f, 1.0f, 0.01f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 共有の「何割目」(0〜1)

        // ルートモーション
        REFLECT_FIELD(bool, useRootMotionY, (false), ("Use Root Motion Y")) // Y も動かすか
        REFLECT_FIELD(bool, useRootMotion,  (false), ("Use Root Motion"))   // ルートモーションを使うか
        REFLECT_FIELD(std::string, rootMotionNodeName, (""), ("Root Motion Node"))  // ルートのノード名

        REFLECT_END()


    // --- 再生 --- //
    AnimPlayMode playMode{ AnimPlayMode::Clip }; // 今再生しているものの種類
    bool isPlaying{ true };                      // 再生中か

    // Clip のとき
    std::string currentAnimation;      // 再生中のクリップ名
    float       animationTime{ 0.0f }; // 再生位置

    // Blend のとき
    float blendTrimStartA{ 0.0f };
    float blendTrimEndA{ 1.0f };
    float blendTrimStartB{ 0.0f };
    float blendTrimEndB{ 1.0f };

    // --- クロスフェード --- //
    float crossfadeTime{ 0.0f };             // クロスフェードにかける秒数
    bool  crossfadeSnapshotPending{ false }; // 次の更新で、前のフレームのポーズを保存するか

    // 混ぜる元のポーズ
    std::vector<DirectX::XMFLOAT3> crossfadeFromPositions;
    std::vector<DirectX::XMFLOAT4> crossfadeFromRotations;
    std::vector<DirectX::XMFLOAT3> crossfadeFromScales;

    // --- ルートモーション --- //
    DirectX::XMFLOAT3 rootMotionDelta{ 0.0f, 0.0f, 0.0f }; // このフレームの移動量

    // --- トリミング --- //
    float trimStart{ 0.0f };      // 始まり
    float trimEnd{ -1.0f };       // 終わり
    bool  trimResolved{ false };  // 割合を秒に直したか

    // --- 速度カーブ --- //
    float speedCurveMultiplier{ 1.0f }; // 再生速度に掛ける倍率
};