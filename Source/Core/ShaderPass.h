#pragma once
#include <string>


// シェーダーパス定義
struct ShaderPass
{
	std::string name;         // パス名
	std::string vertexShader; // 頂点シェーダーパス
	std::string pixelShader;  // ピクセルシェーダーパス
	bool enabled{ true };     // 有効/無効フラグ
};