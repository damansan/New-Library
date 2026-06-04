#include "SceneGame.h"
#include <imgui.h>



void SceneGame::Initialize(const SceneContext& context)
{
	renderSystem.Initialize(context.device);
	debugRenderer.Initialize(context.device);

	// Stage生成/初期化
	stage = std::make_unique<Stage>();
	stage->Initialize(registry, context);

	// Player生成/初期化
	player = std::make_unique<Player>();
	player->Initialize(registry, context);

	// Enemy生成/初期化
	enemy = std::make_unique<Enemy>();
	enemy->Initialize(registry, context);
}

void SceneGame::Update(float elapsedTime, const SceneContext& context)
{
	// 未ロードのモデルを自動ロード
	meshLoaderSystem.Update(
		registry,
		context.device,
		*context.resourceManager
	);
	// アニメーション更新
	animationSystem.Update(registry, elapsedTime);



	// Stage 更新
	stage->Update(elapsedTime, registry, context);
	// Player 更新
	player->Update(elapsedTime, registry, context);
	// Enemy 更新
	enemy->Update(elapsedTime, registry, context);
}

void SceneGame::Render(const SceneContext& context)
{
	// CbScene更新
	CbScene cbSceneData{};
	cbSceneData.view = context.view;
	cbSceneData.projection = context.projection;


	// ライト設定（仮の平行光源）
	cbSceneData.lightDirection = { 0.5f, -1.0f, 0.5f, 0.0f };
	cbSceneData.lightColor = { 1.0f,  1.0f, 1.0f, 1.0f };
	cbSceneData.ambientColor = { 0.2f,  0.2f, 0.2f, 1.0f };

	// カメラ位置
	if (context.camera)
	{
		const auto& eye = context.camera->GetEye();
		cbSceneData.cameraPosition = { eye.x, eye.y, eye.z, 0.0f };
	}

	context.deviceContext->UpdateSubresource(
		renderSystem.GetCbScene(), 0, nullptr, &cbSceneData, 0, 0
	);

	// 頂点シェーダーに設定
	context.deviceContext->VSSetConstantBuffers(
		0, 1, renderSystem.GetCbSceneAddress()
	);

	// ピクセルシェーダーにも設定
	context.deviceContext->PSSetConstantBuffers(
		0, 1, renderSystem.GetCbSceneAddress()
	);

	renderSystem.Render(
		registry,
		context.deviceContext,
		context.renderTargetView,
		context.depthStencilView
	);





	debugRenderer.Clear();

	// SkinnedMeshComponent を持つエンティティのボーンを追加
	auto view = registry.view<SkinnedMeshComponent, TransformComponent>();
	view.each([&](auto entity, SkinnedMeshComponent& skinned, TransformComponent& transform)
		{
			if (!skinned.model) return;

			debugRenderer.AddBones(
				skinned.model->GetNodes(),
				transform.GetWorldMatrix(),
				{ 1.0f, 1.0f, 0.0f, 1.0f }  // 黄色
			);
		});

	//// ボーン描画
	//debugRenderer.Render(
	//	context.deviceContext,
	//	context.view,
	//	context.projection
	//);
}
