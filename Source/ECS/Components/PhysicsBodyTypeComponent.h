#pragma once

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