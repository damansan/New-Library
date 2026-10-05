#pragma once
#include <d3d11.h>
#include <DirectXMath.h>
#include "Engine/Resource/ResourceManager.h"
#include "Camera.h"
#include "PhysX.h"

// Scene系に渡すコンテキスト
// 必要なものが増えたらここに追加する
struct SceneContext
{
	ID3D11Device* device = nullptr;               // DX11デバイス
	ID3D11DeviceContext* deviceContext = nullptr;
	ResourceManager* resourceManager = nullptr;   // リソース管理
	PhysX* physX = nullptr;                       // 物理(PhysX)

	ID3D11RenderTargetView* renderTargetView = nullptr;
	ID3D11DepthStencilView* depthStencilView = nullptr;

	// ゲームカメラ。ゲームの処理が書き込み、ゲームの処理が読む
	Camera* camera = nullptr;

	// 描画に使うカメラ。どれを使うかはApp側が決める(ゲームは読むだけ)
	const Camera* renderCamera = nullptr;
	DirectX::XMFLOAT4X4 view = {};       // renderCameraのビュー行列
	DirectX::XMFLOAT4X4 projection = {}; // renderCameraのプロジェクション行列

	UINT screenWidth = 0;
	UINT screenHeight = 0;

	// Gameへの入力を受け付けてよいか
	bool gameInputActive = false;
};