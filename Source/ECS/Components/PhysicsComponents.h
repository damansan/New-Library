#pragma once
#include <PxPhysicsAPI.h>

// 剛体リスト
struct PhysicsBodyTypeComponent
{
    enum class Type
    {
        None,      // 未選択
        Static,    // 静的
        Dynamic,   // 動的
        Character, // キャラクターコントローラー
    };

    Type type = Type::None;
};


// 静的剛体コンポーネント
struct StaticBodyComponent
{
    physx::PxRigidStatic* actor = nullptr;
};

// キャラクターコントローラーコンポーネント
struct CharacterControllerComponent
{
    physx::PxController* controller = nullptr;

    // カプセル形状パラメータ
    float radius = 0.5f;
    float height = 1.0f;
};
// 足場の端での滑り判定設定
struct EdgeSlideComponent
{
    // 方向サンプリング数
    int sampleCount = 8;

    // 滑り判定
    float supportThreshold = 0.5f;

    // 滑る力
    float slideSpeed = 2.0f;

    // サンプル点から下方向にどれだけ判定するか
    float probeDistance = 0.3f;
};