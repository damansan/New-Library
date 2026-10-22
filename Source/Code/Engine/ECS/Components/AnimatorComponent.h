#pragma once
#include <string>
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
    REFLECT_BEGIN(AnimatorComponent,"Animator")

        // 再生関連
        REFLECT_FIELD(float, playSpeed, (1.0f),  ("Play Speed", 0.0f, 3.0f))                 // 再生速度
        REFLECT_FIELD(bool,  loop,      (true),  ("Loop"))                                   // ループフラグ
        REFLECT_FIELD(bool,  isEnd,     (false), ("Is End",     0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 再生終了フラグ

        // ブレンド関連
        REFLECT_FIELD(float, crossfadeRate, (0.0f), ("Cross FadeRate",  0.0f, 1.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // ブレンドの進み具合
        REFLECT_FIELD(bool, isCrossfading, (false), ("Is Cross Fading", 0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // ブレンド中か

        // Blend ノード関連
        REFLECT_FIELD(std::string, blendClipA, (""),   ("Blend Clip A", 0.0f, 0.0f, 0.1f,  "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 材料Aのクリップ名
        REFLECT_FIELD(std::string, blendClipB, (""),   ("Blend Clip B", 0.0f, 0.0f, 0.1f,  "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 材料Bのクリップ名
        REFLECT_FIELD(float, blendWeight,      (0.0f), ("Blend Weight", 0.0f, 1.0f, 0.01f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // Bの割合
        REFLECT_FIELD(float, blendPhase,       (0.0f), ("Blend Phase",  0.0f, 1.0f, 0.01f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 共有の「何割目」

        // ルートモーション関連
        REFLECT_FIELD(bool, useRootMotionY,            (false), ("Use Root Motion Y")) // Y軸適応
        REFLECT_FIELD(bool, useRootMotion,             (false), ("Use Root Motion"))   // ルートモーション使用
        REFLECT_FIELD(std::string, rootMotionNodeName, (""),    ("Root Motion Node"))  // ルートノード名
      
    REFLECT_END()


    // 再生関連
    std::string currentAnimation;      // 現在のアニメーション名
    float       animationTime{ 0.0f }; // 現在の再生時間
    bool        isPlaying{ true };     // 再生中フラグ

    AnimPlayMode playMode{ AnimPlayMode::Clip }; // 今再生しているものの種類

    // Blend の材料のトリミング範囲
    float blendTrimStartA{ 0.0f };
    float blendTrimEndA{ 1.0f };
    float blendTrimStartB{ 0.0f };
    float blendTrimEndB{ 1.0f };

    // ブレンド関連
    float crossfadeTime{ 0.0f };              // ブレンドにかける時間
    bool  crossfadeSnapshotPending{ false };  // 次の更新で、前のフレームの最終ポーズを保存するか
    
    // 混ぜる元のポーズ
    std::vector<DirectX::XMFLOAT3> crossfadeFromPositions;
    std::vector<DirectX::XMFLOAT4> crossfadeFromRotations;
    std::vector<DirectX::XMFLOAT3> crossfadeFromScales;

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