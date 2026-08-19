#pragma once
#include <string>

struct MeshComponent
{
	std::string modelPath;    // モデルファイルパス
	std::string previousPath; // 前フレームのパスを記録

	MeshComponent() = default;
	MeshComponent(const std::string& path) : modelPath(path) {}
};