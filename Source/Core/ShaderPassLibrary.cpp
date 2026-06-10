#include "ShaderPassLibrary.h"
#include <stdexcept>
#include <vector>

// 静的メンバ定義
std::unordered_map<std::string, ShaderPass> ShaderPassLibrary::passes;
std::vector<std::string> ShaderPassLibrary::names;

// パス登録
void ShaderPassLibrary::Register(const std::string& name, ShaderPass pass)
{
	pass.name = name;
	passes[name] = pass;
	names.push_back(name);
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

// シェーダーパス名一覧取得
const std::vector<std::string>& ShaderPassLibrary::GetNames()
{
	return names;
}