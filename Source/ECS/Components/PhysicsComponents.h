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