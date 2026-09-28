#include "SceneGame.h"
#include <imgui.h>

#include "tracy/Tracy.hpp"

#define WaveDemo 0

void SceneGame::Initialize(const SceneContext& context)
{
	// 剛体コンポーネントの破棄時に、PhysXの実体を自動で破棄予約するようにする
	PhysicsSystem::ConnectDestroyListeners(registry);
	// 物理シーン初期化
	InitializePhysicsScene(*context.physX);

	// 描画システム初期化
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
	physicsDebugRenderer.Initialize(context.device);


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

	gameCameraSystem.Initialize(context.device);

#if WaveDemo && !defined(_DEBUG)
	// デモ初期化
	waveDemo.Initialize(context.device, registry, 500, 1.0f);
#endif
}

void SceneGame::Finalize()
{
	// 物理シーン終了処理
	FinalizePhysicsScene();
}

void SceneGame::Update(float elapsedTime, const SceneContext& context)
{
	ZoneScoped;

	// F2で物理デバッグ描画の表示切り替え
	static bool prevF2 = false;
	bool currF2 = (GetAsyncKeyState(VK_F2) & 0x8000) != 0;
	if (currF2 && !prevF2)
	{
		DrawDebug = !DrawDebug;
	}
	prevF2 = currF2;

	// デモ更新
	waveDemo.Update(elapsedTime);

	// PhysX更新
	{
		ZoneScopedN("UpdatePhysX");
		UpdatePhysicsScene(elapsedTime);
	}

	UpdateGameplay(elapsedTime, context);
	UpdateAnimation(elapsedTime, context);
	UpdateResources(elapsedTime, context);
	UpdateCamera(elapsedTime, context);
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


	// スカイボックス
	{
		ZoneScopedN("SkyBox::Render");
		DirectX::XMFLOAT3 eye = { 0.0f, 0.0f, 0.0f };
		if (context.camera)
		{
			const auto& e = context.camera->GetEye();
			eye = { e.x, e.y, e.z };
		}
		skyBox.Render(context.deviceContext, context.view, context.projection, eye);
	}

	// 描画
	{
		ZoneScopedN("RenderSystem::Render");
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

#if WaveDemo && !defined(_DEBUG)
	// デモ描画
	{
		ZoneScopedN("WaveDemo Loop+Render");

		const auto& lightDir4 = lightManager.GetDirectionalLight().direction;
		DirectX::XMFLOAT3 lightDir3 = { lightDir4.x, lightDir4.y, lightDir4.z };

		waveDemo.Render(
			registry,
			context.deviceContext,
			context.view,
			context.projection,
			lightDir3
		);
	}
#endif

	if (DrawDebug)
	{
		physicsDebugRenderer.Render(
			context.deviceContext,
			GetPhysicsScene(),
			GetControllerManager(),
			context.view,
			context.projection,
			lightManager.GetDirectionalLight().direction);

		DirectX::XMFLOAT4 lightDir4 = lightManager.GetDirectionalLight().direction;
		DirectX::XMFLOAT3 lightDir3 = { lightDir4.x, lightDir4.y, lightDir4.z };
		gameCameraSystem.DrawDebug(
			registry,
			context.deviceContext,
			context.view,
			context.projection,
			lightDir3);
	}
}

void SceneGame::RenderImGui()
{
#ifdef _DEBUG
	//auto view = registry.view<ControllerComponent>();
	//for (auto entity : view)
	//{
	//	auto& ctrl = view.get<ControllerComponent>(entity);
	//	adaptiveTriggerToolWindow.Draw(ctrl);
	//	break;
	//}
#endif
}

// ゲームプレイ系
void SceneGame::UpdateGameplay(float elapsedTime, const SceneContext& context)
{
	// コントローラー入力
	{
		ZoneScopedN("ControllerSystem");
		controllerSystem.Update(registry, elapsedTime);
	}
	// 入力変換
	{
		ZoneScopedN("InputSystem");
		gameInputSystem.Update(registry, controllerSystem.GetSettings());
	}
	// 遷移グラフの評価
	{
		ZoneScopedN("TransitionControllerSystem");
		transitionControllerSystem.Update(registry, elapsedTime, context);
	}

	// プレイヤー待機処理
	{
		ZoneScopedN("PlayerIdleSystem");
		playerIdleSystem.Update(registry, elapsedTime, context);
	}
	// プレイヤー移動処理
	{
		ZoneScopedN("PlayerMoveSystem");
		playerMoveSystem.Update(registry, elapsedTime, context);
	}
	// プレイヤー回避処理
	{
		ZoneScopedN("PlayerDodgeSystem");
		playerDodgeSystem.Update(registry, elapsedTime, context);
	}
	// プレイヤースプリント処理
	{
		ZoneScopedN("PlayerSprintSystem");
		playerSprintSystem.Update(registry, elapsedTime, context);
	}

	// 移動処理
	{
		ZoneScopedN("MovementSystem");
		movementSystem.Update(registry, elapsedTime, context);
	}
}

// アニメーション系
void SceneGame::UpdateAnimation(float elapsedTime, const SceneContext& context)
{
	// State遷移判定
	{
		ZoneScopedN("AnimatorControllerSystem");
		animatorControllerSystem.Update(registry);
	}
	// アニメーション速度更新
	{
		ZoneScopedN("AnimationSpeedSystem");
		animationSpeedSystem.Update(registry);
	}
	// アニメーション更新
	{
		ZoneScopedN("AnimationSystem");
		animationSystem.Update(registry, elapsedTime);
	}
	// シーケンサー更新
	{
		ZoneScopedN("SequencerSystem");
		sequencerSystem.Update(registry);
	}
	// IK更新
	{
		ZoneScopedN("IKSystem");
		ikSystem.Update(registry, GetPhysicsScene());
	}
}

// リソース系
void SceneGame::UpdateResources(float elapsedTime, const SceneContext& context)
{
	// モデルロード
	{
		ZoneScopedN("MeshLoaderSystem");
		meshLoaderSystem.Update(registry, context.device, *context.resourceManager);
	}
	// 物理シーン更新
	{
		// 物理Body生成・削除処理
		ZoneScopedN("PhysicsSystem");
		physicsSystem.Update(registry, *context.physX, GetPhysicsScene(), GetControllerManager(), elapsedTime);
	}
}

// カメラ
void SceneGame::UpdateCamera(float elapsedTime, const SceneContext& context)
{
	ZoneScopedN("CameraSystem");
	if (context.cameraMode == CameraMode::Debug)
	{
		debugCameraSystem.Update(registry, elapsedTime, context.gameInputActive);
		debugCameraSystem.ApplyToCamera(registry, *context.camera);

		// スティック入力があればGameモードに戻す
		if (context.editorContext)
		{
			auto inputView = registry.view<GameInputComponent>();
			for (auto entity : inputView)
			{
				auto& input = inputView.get<GameInputComponent>(entity);
				const float threshold = 0.2f;
				if (fabsf(input.moveStick.x) > threshold ||
					fabsf(input.moveStick.y) > threshold ||
					fabsf(input.cameraStick.x) > threshold ||
					fabsf(input.cameraStick.y) > threshold)
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
