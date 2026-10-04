#include "ProjectComponents.h"
#include "Engine/Resource/ComponentRegistry.h"

#include "Project/ECS/Components/GameInputComponent.h"
#include "Project/ECS/Components/GameCameraComponent.h"
#include "Project/ECS/Components/PlayerIdleComponent.h"
#include "Project/ECS/Components/PlayerMoveComponent.h"
#include "Project/ECS/Components/PlayerDodgeComponent.h"
#include "Project/ECS/Components/PlayerSprintComponent.h"
#include "Project/ECS/Components/PlayerJumpComponent.h"

void ProjectComponents::Register()
{
	REGISTER_COMPONENT_TYPE(GameInputComponent,    "GameInput");
	REGISTER_COMPONENT_TYPE(GameCameraComponent,   "GameCamera");
	REGISTER_COMPONENT_TYPE(PlayerIdleComponent,   "Player Idle");
	REGISTER_COMPONENT_TYPE(PlayerMoveComponent,   "Player Move");
	REGISTER_COMPONENT_TYPE(PlayerDodgeComponent,  "Player Dodge");
	REGISTER_COMPONENT_TYPE(PlayerSprintComponent, "Player Sprint");
	REGISTER_COMPONENT_TYPE(PlayerJumpComponent,   "Player Jump");
}