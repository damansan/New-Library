#pragma once
#include <vector>
#include <string>

#include "Core/ShaderPass.h"
#include "Core/ShaderPassLibrary.h"


struct MaterialComponent
{
	// パスリスト
	std::vector<ShaderPass> passes;

	// パス追加（メソッドチェーン対応）
	MaterialComponent& AddPass(const std::string& passName)
	{
		_ASSERT_EXPR(ShaderPassLibrary::Has(passName), L"登録されていないShaderPassです");
		passes.push_back(ShaderPassLibrary::Get(passName));
		return *this;
	}


	// パスの有効/無効切り替え
	void SetPassEnabled(const std::string& passName, bool enabled)
	{
		for (auto& pass : passes)
		{
			if (pass.name == passName)
			{
				pass.enabled = enabled;
				return;
			}
		}
	}


	// 有効なパスのみ取得
	std::vector<ShaderPass> GetEnabledPasses() const
	{
		std::vector<ShaderPass> result;
		for (const auto& pass : passes)
		{
			if (pass.enabled) result.push_back(pass);
		}
		return result;
	}
};