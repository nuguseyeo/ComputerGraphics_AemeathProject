#include "CharacterController.h"
#include "GameConfig.h"
#include <cmath>

namespace {
float NormalizeDegrees(float degrees) {
    degrees = std::fmod(degrees, 360.0f);
    if (degrees < 0.0f) {
        degrees += 360.0f;
    }
    return degrees;
}
}

bool MovementInput::hasMovement() const {
    return localRight != 0.0f || localForward != 0.0f;
}

MovementInput CharacterController::buildMovementInput(bool forward, bool backward, bool left, bool right, const ThirdPersonCameraController& camera) const {
    MovementInput movement;
    if (forward) movement.localForward += 1.0f;
    if (backward) movement.localForward -= 1.0f;
    if (left) movement.localRight -= 1.0f;
    if (right) movement.localRight += 1.0f;

    const vmath::vec3 cameraForward = camera.forwardXZ();
    const vmath::vec3 cameraRight = camera.rightXZ();
    movement.worldX = cameraRight[0] * movement.localRight + cameraForward[0] * movement.localForward;
    movement.worldZ = cameraRight[2] * movement.localRight + cameraForward[2] * movement.localForward;
    return movement;
}

bool CharacterController::update(Character& character, const MovementInput& movement, const ThirdPersonCameraController& camera, CameraViewMode viewMode, bool sprintHeld, float deltaTime) const {
    const bool hasMovement = movement.hasMovement();
    if (sprintHeld && hasMovement && !character.getIsDashing()) {
        character.setSprinting(true);
    }
    else {
        character.setSprinting(false);
    }

    if (hasMovement) {
        character.move(movement.worldX, movement.worldZ, deltaTime);
    }

    if (viewMode == CameraViewMode::FirstPerson) {
        character.rotateToward(characterFirstPersonYawFromCamera(camera.yawDegrees), deltaTime);
    }
    else if (hasMovement || character.getIsDashing()) {
        const float targetYaw = hasMovement
            ? characterYawFromMovementInput(camera.yawDegrees, movement.localRight, movement.localForward, camera.isOrbitFollowing())
            : characterBackYawFromCamera(camera.yawDegrees);
        character.rotateToward(targetYaw, deltaTime);
    }

    character.updatePhysics(deltaTime);
    return hasMovement || character.getY() > 0.0f || character.getIsDashing();
}

void CharacterController::dash(Character& character, const MovementInput& movement, const ThirdPersonCameraController& camera, CameraViewMode viewMode) const {
    if (movement.hasMovement()) {
        const CharacterMovementConfig config;
        const float targetYaw = viewMode == CameraViewMode::FirstPerson
            ? characterFirstPersonYawFromCamera(camera.yawDegrees)
            : characterYawFromMovementInput(camera.yawDegrees, movement.localRight, movement.localForward, camera.isOrbitFollowing());
        character.rotateToward(targetYaw, config.dashFacingDeltaTime);
    }
    character.dash(movement.worldX, movement.worldZ);
}

void CharacterController::alignToCamera(Character& character, const ThirdPersonCameraController& camera, CameraViewMode viewMode, float deltaTime) const {
    const float targetYaw = viewMode == CameraViewMode::FirstPerson
        ? characterFirstPersonYawFromCamera(camera.yawDegrees)
        : characterBackYawFromCamera(camera.yawDegrees);
    character.rotateToward(targetYaw, deltaTime);
}

float CharacterController::characterBackYawFromCamera(float cameraYawDegrees) const {
    return NormalizeDegrees(180.0f - cameraYawDegrees);
}

float CharacterController::characterFirstPersonYawFromCamera(float cameraYawDegrees) const {
    return characterBackYawFromCamera(cameraYawDegrees);
}

float CharacterController::characterYawFromMovementInput(float cameraYawDegrees, float localRight, float localForward, bool lateralFollowsCameraOrbit) const {
    const float backYaw = characterBackYawFromCamera(cameraYawDegrees);

    if (localForward < 0.0f) {
        return NormalizeDegrees(backYaw + 180.0f);
    }
    if (localForward == 0.0f && localRight < 0.0f && !lateralFollowsCameraOrbit) {
        return NormalizeDegrees(backYaw + 90.0f);
    }
    if (localForward == 0.0f && localRight > 0.0f && !lateralFollowsCameraOrbit) {
        return NormalizeDegrees(backYaw - 90.0f);
    }

    return backYaw;
}
