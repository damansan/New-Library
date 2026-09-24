#pragma once
#include "entt.hpp"
#include "Tool/Editor/MetaRegistry.h"

using namespace entt::literals;

// 型そのものの登録（TypeInfoの付与）を開始する
#define REFLECT_BEGIN(Type, DisplayName)                                            \
private:                                                                            \
    using ReflectSelf = Type;                                                       \
    static inline bool _reflect_type_registered = []                                \
    {                                                                               \
        entt::meta_factory<Type>{}                                                  \
            .type(#Type##_hs)                                                       \
            .custom<TypeInfo>(TypeInfo{ DisplayName });                             \
        return true;                                                                \
    }();                                                                            \
public:

// 1フィールド分の宣言＋登録
#define REFLECT_FIELD(FieldType, FieldName, Label, Min, Max, Step, ...)             \
    FieldType FieldName = { __VA_ARGS__ };                                          \
    static inline bool _reflect_##FieldName##_registered = []                       \
    {                                                                               \
        entt::meta_factory<ReflectSelf>{}                                           \
            .data<&ReflectSelf::FieldName>(#FieldName##_hs)                         \
            .custom<FieldInfo>(FieldInfo{ Label, Min, Max, Step });                 \
        return true;                                                                \
    }();

// 今回は特に何もしないが、対称性のために用意（将来の拡張用）
#define REFLECT_END()