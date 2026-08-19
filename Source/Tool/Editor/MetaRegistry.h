#pragma once
#include <string>
#include <vector>
#include "entt.hpp"

#include "Engine/Data/AnimatorGraphData.h"

// フィールド1つに添付する付加情報
struct FieldInfo
{
	std::string label;       // Inspector上の表示名
	float minValue  = 0.0f;  // 下限値
	float maxValue  = 0.0f;  // 上限値
	float dragSpeed = 0.1f;  // 
	std::string dragDropPayload;
	bool readOnly = false;
};

// Getter選択UIなどで使う、フィールド1件分の情報
struct FieldEntry
{
	entt::meta_type componentType; // 属するコンポーネントの型
	entt::id_type fieldId = 0;     // entt::meta上のフィールドID(取得・比較に使う)
	std::string componentName;     // 表示用:コンポーネント名
	std::string category;          // 表示用:カテゴリ
	std::string fieldLabel;        // 表示用:フィールド名
	ValueType type = ValueType::Float;
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

	// 登録済みのフィード一覧を取得する
	static const std::vector<FieldEntry>& GetAllFields();

	// Float2/3/4型の、成分数を返す(それ以外は0)
	static int GetComponentCount(ValueType type);
};