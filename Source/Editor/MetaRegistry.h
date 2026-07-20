#pragma once
#include <string>
#include "entt.hpp"

// フィールド1つに添付する付加情報
struct FieldInfo
{
	std::string label;       // Inspector上の表示名
	float minValue  = 0.0f;  // 下限値
	float maxValue  = 0.0f;  // 上限値
	float dragSpeed = 0.1f;
	std::string dragDropPayload;
	bool readOnly = false;
};

// コンポーネント1つに添付する付加情報
struct TypeInfo
{
	std::string category;
};

class MetaRegistry
{
public:
	// メタデータの初期化
	static void Initialize();

	// 登録済みメタ情報からInspectorUIを自動描画
	static void DrawMetaFields(entt::meta_type metaType, entt::meta_any instance);
};