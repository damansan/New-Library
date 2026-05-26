#pragma once
#include <string>

struct MeshComponent
{
	std::string modelPath; // モデルファイルパス
	bool visible{true};    // 表示フラグ
	bool castShadow{true}; // 影を落とすか

	MeshComponent() = default;
	MeshComponent(const std::string& path) : modelPath(path) {}
};