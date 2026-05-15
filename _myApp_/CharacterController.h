#pragma once

#include "Camera.h"
#include "Character.h"
#include "ThirdPersonCameraController.h"
#include <vmath.h>

// MovementBasis는 CharacterController가 카메라 컨트롤러 타입에 직접 묶이지 않도록 준비하는 입력 기준입니다.
// 기존 ThirdPersonCameraController에서 필요한 값만 복사해 전달할 수 있습니다.
struct MovementBasis {
    vmath::vec3 forwardXZ = vmath::vec3(0.0f, 0.0f, -1.0f);
    vmath::vec3 rightXZ = vmath::vec3(1.0f, 0.0f, 0.0f);
    float yawDegrees = 0.0f;
    bool orbitFollowing = false;
};

// 한 프레임의 이동 입력을 로컬 입력값과 월드 방향값으로 함께 보관합니다.
struct MovementInput {
    float localRight = 0.0f;
    float localForward = 0.0f;
    float worldX = 0.0f;
    float worldZ = 0.0f;

    bool hasMovement() const;
};

// 입력과 카메라 상태를 Character의 이동, 회전, 대쉬 호출로 변환합니다.
class CharacterController {
public:
    MovementInput buildMovementInput(bool forward, bool backward, bool left, bool right, const MovementBasis& basis) const;
    MovementInput buildMovementInput(bool forward, bool backward, bool left, bool right, const ThirdPersonCameraController& camera) const;

    bool update(Character& character, const MovementInput& movement, const MovementBasis& basis, CameraViewMode viewMode, bool sprintHeld, float deltaTime) const;
    bool update(Character& character, const MovementInput& movement, const ThirdPersonCameraController& camera, CameraViewMode viewMode, bool sprintHeld, float deltaTime) const;

    void dash(Character& character, const MovementInput& movement, const MovementBasis& basis, CameraViewMode viewMode) const;
    void dash(Character& character, const MovementInput& movement, const ThirdPersonCameraController& camera, CameraViewMode viewMode) const;

    void alignToCamera(Character& character, const MovementBasis& basis, CameraViewMode viewMode, float deltaTime) const;
    void alignToCamera(Character& character, const ThirdPersonCameraController& camera, CameraViewMode viewMode, float deltaTime) const;

private:
    MovementBasis makeMovementBasis(const ThirdPersonCameraController& camera) const;
    float characterBackYawFromCamera(float cameraYawDegrees) const;
    float characterFirstPersonYawFromCamera(float cameraYawDegrees) const;
    float characterYawFromMovementInput(float cameraYawDegrees, float localRight, float localForward, bool lateralFollowsCameraOrbit) const;
};