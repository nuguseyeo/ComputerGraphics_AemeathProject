#pragma once

#include <vmath.h>
#include "Camera.h"

// ModelDrawParams는 App/Renderer 쪽 프레임 상태를 Model::draw에 전달하는 입력 묶음입니다.
// Model은 이 값만 읽고 게임 입력이나 카메라 컨트롤러에는 직접 의존하지 않습니다.
struct ModelDrawParams {
    float currentTime = 0.0f;
    float deltaTime = 0.0f;
    float aspect = 1.0f;
    vmath::vec3 characterPosition = vmath::vec3(0.0f, 0.0f, 0.0f);
    float characterYawDegrees = 0.0f;
    CameraState camera;
    bool hasMovementInput = false;
    float movementSpeedScale = 0.0f;
};