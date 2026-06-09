#include "Player.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/MeshComponent.h"
#include "ECS/Components/MaterialComponent.h"
#include "ECS/Systems/AnimationSystem.h"
#include "ECS/Components/InputComponent.h"   
#include "ECS/Components/MovementComponent.h"

#include "imgui.h"

void Player::Initialize(entt::registry& registry, const SceneContext& context)
{
	entity = registry.create();
	reg = &registry;

	auto& t = registry.emplace<TransformComponent>(entity);
	t.position = { 0.0f, 0.0f, 0.0f };
	t.scale = { 1.0f, 1.0f, 1.0f };

	registry.emplace<MeshComponent>(entity, "Data/Model/PlayerRobot/Map_Robot3.gltf");
	//registry.emplace<MeshComponent>(entity, "Data/Model/unitychan.glb");
	registry.emplace<MaterialComponent>(entity).AddPass("pbr");
	registry.emplace<InputComponent>(entity);
	registry.emplace<MovementComponent>(entity);
}

void Player::Update(float elapsedTime, entt::registry& registry, const SceneContext& context)
{
	auto* animator = registry.try_get<AnimatorComponent>(entity);
	if (animator)
	{
		animator->currentAnimation = "Idle";
		animator->loop = true;
		animator->playSpeed = 1.0f;
	}
}

void Player::RenderImGui()
{
	if (!reg) return;
	auto* transform = reg->try_get<TransformComponent>(entity);
	if (!transform) return;

	ImGui::Begin("Player Info");

	ImGui::Text("Position");
	ImGui::DragFloat3("##Position", &transform->position.x, 0.01f);

	ImGui::Text("Scale");
	ImGui::DragFloat3("##Scale", &transform->scale.x, 0.0001f);

	ImGui::Text("Angle");
	ImGui::DragFloat3("##Angle", &transform->rotation.x, 0.01f);


	ImGui::End();
}