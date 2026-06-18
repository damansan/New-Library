#include "SceneGame.h"
#include <imgui.h>

#include "tracy/Tracy.hpp"

void SceneGame::Initialize(const SceneContext& context)
{
	renderSystem.Initialize(context.device);

	// SkyBox初期化
	skyBox.Initialize(
		context.device,
		"Data/SkyBox/BlueSky"
		//"Data/SkyBox/DynamicSky"
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
}

void SceneGame::Update(float elapsedTime, const SceneContext& context)
{
	ZoneScoped;

	{
		// コントローラー入力
		ZoneScopedN("ControllerSystem");
		controllerSystem.Update(registry, elapsedTime);
	}

	{
		// 入力変換
		ZoneScopedN("InputSystem");
		inputSystem.Update(registry, controllerSystem.GetSettings());
	}
	{
		// 移動
		ZoneScopedN("MovementSystem");
		movementSystem.Update(registry, elapsedTime, context);
	}
	{
		// アニメーション更新
		ZoneScopedN("AnimationSystem");
		animationSystem.Update(registry, elapsedTime);
	}

	{
		// モデルロード
		ZoneScopedN("MeshLoaderSystem");
		meshLoaderSystem.Update(registry, context.device, *context.resourceManager);
	}


	{
		// カメラ
		ZoneScopedN("CameraSystem");
		if (context.cameraMode == CameraMode::Debug)
		{
			debugCameraSystem.Update(registry, elapsedTime);
			debugCameraSystem.ApplyToCamera(registry, *context.camera);

			// スティック入力があればGameモードに戻す
			if (context.editorContext)
			{
				auto inputView = registry.view<InputComponent>();
				for (auto entity : inputView)
				{
					auto& input = inputView.get<InputComponent>(entity);
					const float threshold = 0.2f;
					if (fabsf(input.moveX) > threshold ||
						fabsf(input.moveZ) > threshold ||
						fabsf(input.cameraX) > threshold ||
						fabsf(input.cameraY) > threshold)
					{
						context.editorContext->cameraMode = CameraMode::Game;
						break;
					}
				}
			}
		}
		else
		{
			gameCameraSystem.Update(registry, elapsedTime);
			gameCameraSystem.ApplyToCamera(registry, *context.camera);
		}
	}
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

	// スカイボックスをLightingPass後・半透明パス前に差し込む
	{
		DirectX::XMFLOAT3 eye = { 0.0f, 0.0f, 0.0f };
		if (context.camera)
		{
			const auto& e = context.camera->GetEye();
			eye = { e.x, e.y, e.z };
		}
		renderSystem.SetPreTransparentCallback([this, dc = context.deviceContext,
			view = context.view, proj = context.projection, eye]()
		{
			skyBox.Render(dc, view, proj, eye);
		});
	}

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

}


void SceneGame::RenderImGui()
{
#ifdef _DEBUG
	//player->RenderImGui();
	controllerToolWindow.Draw(registry, controllerSystem);
#endif
}
