#include "CharacterController.h"
#include "GameConfig.h"
#include <cmath>

#pragma region Static Helpers
// 이 cpp 내부에서만 쓰는 보조 함수와 상수를 모은 구역입니다.

namespace {
float NormalizeDegrees(float degrees) {
    degrees = std::fmod(degrees, 360.0f);
    if (degrees < 0.0f) {
        degrees += 360.0f;
    }
    return degrees;
}
}

#pragma endregion

#pragma region Movement Input
// 키 입력 상태를 카메라 기준 로컬/월드 이동 벡터로 변환하는 구역입니다.

bool MovementInput::hasMovement() const {
    return localRight != 0.0f || localForward != 0.0f;
}

MovementInput CharacterController::buildMovementInput(bool forward, bool backward, bool left, bool right, const MovementBasis& basis) const {
    MovementInput movement;
    if (forward) movement.localForward += 1.0f;
    if (backward) movement.localForward -= 1.0f;
    if (left) movement.localRight -= 1.0f;
    if (right) movement.localRight += 1.0f;

    movement.worldX = basis.rightXZ[0] * movement.localRight + basis.forwardXZ[0] * movement.localForward;
    movement.worldZ = basis.rightXZ[2] * movement.localRight + basis.forwardXZ[2] * movement.localForward;

    // Character::move()와 Character::dash()가 최종 방향을 정규화하므로 여기서는 중복 정규화하지 않습니다.
    return movement;
}

MovementInput CharacterController::buildMovementInput(bool forward, bool backward, bool left, bool right, const ThirdPersonCameraController& camera) const {
    return buildMovementInput(forward, backward, left, right, makeMovementBasis(camera));
}

#pragma endregion

#pragma region Character Update
// MovementInput과 CameraViewMode를 Character 이동/회전/대쉬/물리 호출로 적용하는 구역입니다.

bool CharacterController::update(Character& character, const MovementInput& movement, const MovementBasis& basis, CameraViewMode viewMode, bool sprintHeld, float deltaTime) const {
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
        character.rotateToward(characterFirstPersonYawFromCamera(basis.yawDegrees), deltaTime);
    }
    else if (hasMovement || character.getIsDashing()) {
        const float targetYaw = hasMovement
            ? characterYawFromMovementInput(basis.yawDegrees, movement.localRight, movement.localForward, basis.orbitFollowing)
            : characterBackYawFromCamera(basis.yawDegrees);
        character.rotateToward(targetYaw, deltaTime);
    }

    character.updatePhysics(deltaTime);
    return hasMovement || character.getY() > 0.0f || character.getIsDashing();
}

bool CharacterController::update(Character& character, const MovementInput& movement, const ThirdPersonCameraController& camera, CameraViewMode viewMode, bool sprintHeld, float deltaTime) const {
    return update(character, movement, makeMovementBasis(camera), viewMode, sprintHeld, deltaTime);
}

void CharacterController::dash(Character& character, const MovementInput& movement, const MovementBasis& basis, CameraViewMode viewMode) const {
    if (movement.hasMovement()) {
        const CharacterMovementConfig config;
        const float targetYaw = viewMode == CameraViewMode::FirstPerson
            ? characterFirstPersonYawFromCamera(basis.yawDegrees)
            : characterYawFromMovementInput(basis.yawDegrees, movement.localRight, movement.localForward, basis.orbitFollowing);
        character.rotateToward(targetYaw, config.dashFacingDeltaTime);
    }
    character.dash(movement.worldX, movement.worldZ);
}

void CharacterController::dash(Character& character, const MovementInput& movement, const ThirdPersonCameraController& camera, CameraViewMode viewMode) const {
    dash(character, movement, makeMovementBasis(camera), viewMode);
}

void CharacterController::alignToCamera(Character& character, const MovementBasis& basis, CameraViewMode viewMode, float deltaTime) const {
    const float targetYaw = viewMode == CameraViewMode::FirstPerson
        ? characterFirstPersonYawFromCamera(basis.yawDegrees)
        : characterBackYawFromCamera(basis.yawDegrees);
    character.rotateToward(targetYaw, deltaTime);
}

void CharacterController::alignToCamera(Character& character, const ThirdPersonCameraController& camera, CameraViewMode viewMode, float deltaTime) const {
    alignToCamera(character, makeMovementBasis(camera), viewMode, deltaTime);
}

#pragma endregion

#pragma region Yaw Helpers
// 카메라 yaw와 이동 입력을 캐릭터가 바라볼 yaw 값으로 변환하는 보조 함수 구역입니다.

MovementBasis CharacterController::makeMovementBasis(const ThirdPersonCameraController& camera) const {
    MovementBasis basis;
    basis.forwardXZ = camera.forwardXZ();
    basis.rightXZ = camera.rightXZ();
    basis.yawDegrees = camera.yawDegrees;
    basis.orbitFollowing = camera.isOrbitFollowing();
    return basis;
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

#pragma endregion