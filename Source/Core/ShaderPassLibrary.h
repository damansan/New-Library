#pragma once
#include <string>
#include <unordered_map>

#include "ShaderPass.h"

// シェーダーパス登録/取得
class ShaderPassLibrary
{
public:
	// パス登録
	static void Register(const std::string& name, ShaderPass pass);

	// パス取得
	static const ShaderPass& Get(const std::string& name);

	// パス存在確認
	static bool Has(const std::string& name);

private:
	static std::unordered_map<std::string, ShaderPass> passes;
};