#include "SceneGame.h"
#include <imgui.h>

#include "tracy/Tracy.hpp"

void SceneGame::Initialize(const SceneContext& context)
{
	renderSystem.Initialize(context.device);
	debugRenderer.Initialize(context.device);

	// SkyBox初期化
	skyBox.Initialize(
		context.device,
		//"Data/SkyBox/BlueSky"
		"Data/SkyBox/DynamicSky"
	);
	// IBLテクスチャセット
	renderSystem.SetIBLTextures(
		skyBox.GetDiffuseIem(),
		skyBox.GetSRV(),
		skyBox.GetLutGgx()
	);
	lightManager.Initialize(context.device);
	lightManager.SetDirectionalLight(
		{ 0.5f, -1.0f, 0.5f },
		{ 1.0f,  1.0f, 1.0f },
		1.0f
	);

	//lightManager.AddPointLight(
	//	{ 0.0f, 1.0f, 2.0f }, // 位置
	//	{ 1.0f, 0.0f, 0.0f }, // 色（赤）
	//	5.0f,                  // 範囲
	//	3.0f                   // 強度
	//);

	const int lightCount = 20;
	const float radius = 5.0f;
	const float height = 1.0f;

	for (int i = 0; i < lightCount; i++)
	{
		float angle = (2.0f * 3.14159265f / lightCount) * i;
		float x = radius * cosf(angle);
		float z = radius * sinf(angle);

		float r = (sinf(angle) + 1.0f) * 0.5f;
		float g = (sinf(angle + 2.0f) + 1.0f) * 0.5f;
		float b = (sinf(angle + 4.0f) + 1.0f) * 0.5f;

		lightManager.AddPointLight(
			{ x, height, z }, // 位置
			{ r, g, b },      // 色
			5.0f,             // 範囲
			3.0f              // 強度
		);
	}


	// Stage生成/初期化
	stage = std::make_unique<Stage>();
	stage->Initialize(registry, context);

	// Player生成/初期化
	player = std::make_unique<Player>();
	player->Initialize(registry, context);

	//// Enemy生成/初期化
	//enemy = std::make_unique<Enemy>();
	//enemy->Initialize(registry, context);
}

void SceneGame::Update(float elapsedTime, const SceneContext& context)
{
	ZoneScoped;
	{
		ZoneScopedN("InputSystem");
		inputSystem.Update(registry);
	}
	{
		ZoneScopedN("MovementSystem");
		movementSystem.Update(registry, elapsedTime, context);
	}
	{
		ZoneScopedN("MeshLoaderSystem");

		// 未ロードのモデルを自動ロード
		meshLoaderSystem.Update(registry, context.device, *context.resourceManager);
	}

	{
		ZoneScopedN("AnimationSystem");

		// アニメーション更新
		animationSystem.Update(registry, elapsedTime);
	}



	{
		ZoneScopedN("Stage::Update");

		// Stage 更新
		stage->Update(elapsedTime, registry, context);
	}
	{
		ZoneScopedN("Player::Update");

		// Player 更新
		player->Update(elapsedTime, registry, context);
	}
	//{
	//	ZoneScopedN("Enemy::Update");

	//	// Enemy 更新
	//	enemy->Update(elapsedTime, registry, context);
	//}
}

void SceneGame::Render(const SceneContext& context)
{
	ZoneScoped;

	{
		ZoneScopedN("CbScene Update");

		// CbScene更新
		CbScene cbSceneData{};
		cbSceneData.view = context.view;
		cbSceneData.projection = context.projection;

		// カメラ位置
		if (context.camera)
		{
			const auto& eye = context.camera->GetEye();
			cbSceneData.cameraPosition = { eye.x, eye.y, eye.z, 0.0f };
		}
		lightManager.Bind(context.deviceContext);

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
	}

	// フラスタム計算
	renderSystem.SetFrustum(context.view, context.projection);

	{
		ZoneScopedN("RenderSystem::Render");

		// 描画
		renderSystem.Render(
			registry,
			context.deviceContext,
			context.renderTargetView,
			context.depthStencilView,
			context.device,
			context.screenWidth,
			context.screenHeight
		);
	}

	{
		ZoneScopedN("SkyBox::Render");

		// カメラ位置を取得
		DirectX::XMFLOAT3 eye = { 0.0f, 0.0f, 0.0f };
		if (context.camera)
		{
			const auto& e = context.camera->GetEye();
			eye = { e.x, e.y, e.z };
		}

		skyBox.Render(
			context.deviceContext,
			context.view,
			context.projection,
			eye
		);
	}


	{
		ZoneScopedN("DebugRenderer");

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

	}

	{
		ZoneScopedN("DebugRenderer::Render");

		// ボーン描画
		//debugRenderer.Render(
		//	context.deviceContext,
		//	context.view,
		//	context.projection
		//);
	}
}
