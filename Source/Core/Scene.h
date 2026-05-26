#pragma once
#include "entt.hpp"
#include "Core/SceneContext.h"
#include "ECS/Systems/MeshLoaderSystem.h"

class Scene
{
public:
	Scene() = default;
	virtual ~Scene() = default;

	Scene(const Scene&) = delete;                // コピーコンストラクタ禁止
	Scene& operator=(const Scene&) = delete;     // コピー代入禁止
	Scene(Scene&&) noexcept = delete;            // ムーブコンストラクタ禁止
	Scene& operator=(Scene&&) noexcept = delete; // ムーブ代入禁止

	// 初期化
	virtual void Initialize(const SceneContext& context) = 0;
	// 終了化
	virtual void Finalize() = 0;
	// 更新
	virtual void Update(float elapsedTime, const SceneContext& context) = 0;
	// 描画
	virtual void Render(const SceneContext& context) = 0;
	// ImGui描画
	virtual void RenderImGui() = 0;

	// レジストリ取得
	entt::registry& GetRegistry() { return registry; }

protected:
	entt::registry registry; // Entity/Component管理
	MeshLoaderSystem meshLoaderSystem;
};