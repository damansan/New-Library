#pragma once

// エンティティの属性を付与するコンポーネント
enum class EntityCategory
{
    None, Player, Enemy, Stage, Object, Camera
};

struct ComponentCategory
{
    EntityCategory category = EntityCategory::None;
};