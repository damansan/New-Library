#include "ProjectInspectors.h"
#include "Tool/Editor/ComponentInspectorRegistry.h"
#include "Project/ECS/Components/GameCameraComponent.h"

#include <imgui.h>
#include <string>

void ProjectInspectors::Register()
{
	// ゲームカメラ
	REGISTER_INSPECTOR(GameCameraComponent,
		[](auto& reg, auto e)
		{
			auto* _c = reg.try_get<GameCameraComponent>(e);
			if (!_c) return;
			if (!ImGui::CollapsingHeader("GameCamera", ImGuiTreeNodeFlags_DefaultOpen)) return;

			const char* modeNames[] = { "Follow", "LookAt" };
			int mode = (int)_c->mode;
			if (ImGui::Combo("Mode", &mode, modeNames, 2))
				_c->mode = (Mode)mode;

			// FollowTargetは自動で探す(Playerタグ)
			std::string followTargetName = "None (auto)";
			if (_c->followTarget != entt::null && reg.valid(_c->followTarget))
				followTargetName = "Entity " + std::to_string((uint32_t)_c->followTarget);

			ImGui::Text("Follow Target: %s", followTargetName.c_str());

			// LookAtTargetは自動で探す(Enemyタグ)
			std::string lookAtTargetName = "None (auto)";
			if (_c->lookAtTarget != entt::null && reg.valid(_c->lookAtTarget))
				lookAtTargetName = "Entity " + std::to_string((uint32_t)_c->lookAtTarget);

			ImGui::Text("LookAt Target: %s", lookAtTargetName.c_str());

			ImGui::DragFloat("Yaw", &_c->yaw, 0.01f);
			ImGui::DragFloat("Pitch", &_c->pitch, 0.01f, -1.5f, 1.5f);

			// --- Follow Camera ---
			ImGui::Separator();
			ImGui::Text("Follow Camera");
			ImGui::DragFloat("Distance", &_c->followCameraDistance, 0.1f, 0.0f, 50.0f);
			ImGui::DragFloat("Height", &_c->followCameraHeight, 0.1f, 0.0f, 20.0f);
			ImGui::DragFloat("Target Y Up", &_c->followCameraTargetYUp, 0.1f, 0.0f, 10.0f);
			ImGui::DragFloat("Eye Speed Ratio", &_c->followCameraEyeSpeedRatio, 0.5f, 0.0f, 100.0f);
			ImGui::DragFloat("Target Speed Ratio", &_c->followCameraTargetSpeedRatio, 0.5f, 0.0f, 100.0f);

			// --- LockOn Camera（共通） ---
			ImGui::Separator();
			ImGui::Text("LockOn Camera");
			ImGui::DragFloat("Eye Speed Ratio##LockOn", &_c->lockOnCameraEyeSpeedRatio, 0.5f, 0.0f, 100.0f);
			ImGui::DragFloat("Target Speed Ratio##LockOn", &_c->lockOnCameraTargetSpeedRatio, 0.5f, 0.0f, 100.0f);
			ImGui::DragFloat("Range Switch Distance", &_c->lockOnRangeSwitchDistance, 0.1f, 0.1f, 50.0f);

			// --- 円の中心O ---
			ImGui::Separator();
			ImGui::Text("Center O");
			ImGui::DragFloat("Center Ratio", &_c->lockOnCenterRatio, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat("Far Center Max Distance", &_c->lockOnFarCenterMaxDistance, 0.05f, 0.0f, 20.0f);
			ImGui::DragFloat("Target Circle Radius Max", &_c->lockOnTargetCircleRadiusMax, 0.1f, 0.0f, 20.0f);

			// --- 円周上の角度・スティック操作 ---
			ImGui::Separator();
			ImGui::Text("Orbit");
			ImGui::DragFloat("Orbit Rotate Speed", &_c->lockOnOrbitRotateSpeed, 0.1f, 0.0f, 20.0f);
			ImGui::DragFloat("Orbit Pitch Rotate Speed", &_c->lockOnOrbitPitchRotateSpeed, 0.1f, 0.0f, 20.0f);
			ImGui::DragFloat("Orbit Return Speed", &_c->lockOnOrbitReturnSpeed, 0.5f, 0.0f, 50.0f);
			ImGui::Separator();
			ImGui::Text("Orbit");
			ImGui::DragFloat("Orbit Angle", &_c->lockOnOrbitAngle, 0.01f);
			ImGui::DragFloat("Orbit Pitch", &_c->lockOnOrbitPitch, 0.01f, -0.8f, 0.8f);

			// --- くの字の向き ---
			ImGui::Separator();
			ImGui::Text("Side");
			ImGui::DragFloat("Side Switch Threshold", &_c->lockOnSideSwitchThreshold, 0.01f, 0.0f, 3.14f);
			ImGui::DragFloat("Side Smooth Speed", &_c->lockOnSideSmoothSpeed, 0.5f, 0.0f, 50.0f);
			ImGui::Text("Side : %.2f -> %.2f", _c->lockOnSide, _c->lockOnSideSmooth);

			// --- 近距離用パラメータ ---
			ImGui::Separator();
			ImGui::Text("Near");
			ImGui::DragFloat("Near Distance", &_c->lockOnNearCameraDistance, 0.1f, 0.0f, 30.0f);
			ImGui::DragFloat("Near Height", &_c->lockOnNearCameraHeight, 0.1f, 0.0f, 20.0f);
			ImGui::DragFloat("Near Target Y Up", &_c->lockOnNearCameraTargetYUp, 0.1f, 0.0f, 10.0f);
			ImGui::DragFloat("Near Angle Offset Max", &_c->lockOnNearAngleOffsetMax, 0.5f, 0.0f, 90.0f);

			// --- 遠距離用パラメータ ---
			ImGui::Separator();
			ImGui::Text("Far");
			ImGui::DragFloat("Far Distance", &_c->lockOnFarCameraDistance, 0.1f, 0.0f, 30.0f);
			ImGui::DragFloat("Far Height", &_c->lockOnFarCameraHeight, 0.1f, 0.0f, 20.0f);
			ImGui::DragFloat("Far Target Y Up", &_c->lockOnFarCameraTargetYUp, 0.1f, 0.0f, 10.0f);

			// --- 減衰ばね ---
			ImGui::Separator();
			ImGui::Text("Spring");
			ImGui::DragFloat("Stiffness", &_c->eyeSpringStiffness, 5.0f, 0.0f, 1000.0f);
			ImGui::DragFloat("Damping", &_c->eyeSpringDamping, 1.0f, 0.0f, 200.0f);

			// --- 現在のeye/target（デバッグ表示） ---
			ImGui::Separator();
			ImGui::DragFloat3("Eye", &_c->base.currentEye.x, 0.1f);
			ImGui::DragFloat3("Target", &_c->base.currentTarget.x, 0.1f);
		});
}