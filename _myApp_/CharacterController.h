#pragma once

#include "Character.h"
#include "ThirdPersonCameraController.h"
#include <vmath.h>

struct MovementInput {
    float localRight = 0.0f;
    float localForward = 0.0f;
    float worldX = 0.0f;
    float worldZ = 0.0f;

    bool hasMovement() const;
};

class CharacterController {
public:
    MovementInput buildMovementInput(bool forward, bool backward, bool left, bool right, const ThirdPersonCameraController& camera) const;
    bool update(Character& character, const MovementInput& movement, const ThirdPersonCameraController& camera, CameraViewMode viewMode, bool sprintHeld, float deltaTime) const;
    void dash(Character& character, const MovementInput& movement, const ThirdPersonCameraController& camera, CameraViewMode viewMode) const;
    void alignToCamera(Character& character, const ThirdPersonCameraController& camera, CameraViewMode viewMode, float deltaTime) const;

private:
    float characterBackYawFromCamera(float cameraYawDegrees) const;
    float characterFirstPersonYawFromCamera(float cameraYawDegrees) const;
    float characterYawFromMovementInput(float cameraYawDegrees, float localRight, float localForward, bool lateralFollowsCameraOrbit) const;
};
