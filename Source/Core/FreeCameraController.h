#pragma once
#include <windows.h>
#include <DirectXMath.h>
#include "Camera.h"

class FreeCameraController
{
public:
    FreeCameraController() = default;
    ~FreeCameraController() = default;

    // çXêV
    void Update(float elapsedTime, Camera& camera);

private:
    float cameraSpeed = 5.0f;

    float yaw = 0.0f;
    float pitch = 0.0f;
    bool  firstMouse = true;
    POINT lastMousePos = {};
};