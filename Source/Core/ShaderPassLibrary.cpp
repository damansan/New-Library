#include "ShaderPassLibrary.h"
#include <stdexcept>

// 静的メンバ定義
std::unordered_map<std::string, ShaderPass> ShaderPassLibrary::passes;


// パス登録
void ShaderPassLibrary::Register(const std::string& name, ShaderPass pass)
{
	pass.name = name;
	passes[name] = pass;
}

// パス取得
const ShaderPass& ShaderPassLibrary::Get(const std::string& name)
{
	auto it = passes.find(name);
	_ASSERT_EXPR(it != passes.end(), L"ShaderPass が登録されていません");
	return it->second;
}

// パス存在確認
bool ShaderPassLibrary::Has(const std::string& name)
{
	return passes.find(name) != passes.end();
}
