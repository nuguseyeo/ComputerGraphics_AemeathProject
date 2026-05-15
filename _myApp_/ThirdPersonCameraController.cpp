#include "ThirdPersonCameraController.h"
#include <algorithm>
#include <cmath>

namespace {
constexpr float kDegToRad = 0.0174532925f;

float NormalizeDegrees(float degrees) {
    degrees = std::fmod(degrees, 360.0f);
    if (degrees < 0.0f) {
        degrees += 360.0f;
    }
    return degrees;
}
}

ThirdPersonCameraController::ThirdPersonCameraController(const ThirdPersonCameraConfig& config)
    : pitchDegrees(config.initialPitchDegrees)
    , distance(config.initialDistance)
    , config(config) {
}

void ThirdPersonCameraController::applyMouseDelta(int deltaX, int deltaY) {
    yawDegrees = NormalizeDegrees(yawDegrees + deltaX * config.mouseSensitivity);
    pitchDegrees = std::max(config.minPitch, std::min(config.maxPitch, pitchDegrees - deltaY * config.mouseSensitivity));
}

void ThirdPersonCameraController::applyWheel(int wheelDelta) {
    distance = std::max(config.minDistance, std::min(config.maxDistance, distance - wheelDelta * config.wheelZoomStep));
}

void ThirdPersonCameraController::clampPitchForViewMode(CameraViewMode viewMode) {
    if (viewMode == CameraViewMode::FirstPerson) {
        pitchDegrees = std::max(config.firstPersonMinPitch, std::min(config.firstPersonMaxPitch, pitchDegrees));
    }
    else if (pitchDegrees < config.thirdPersonMinPitch) {
        pitchDegrees = config.thirdPersonMinPitch;
    }
}

void ThirdPersonCameraController::updateOrbitFollow(float deltaTime) {
    orbitFollowTimer = std::max(0.0f, orbitFollowTimer - deltaTime);
}

void ThirdPersonCameraController::notifyOrbitInput() {
    orbitFollowTimer = config.orbitFollowDuration;
}

vmath::vec3 ThirdPersonCameraController::forwardXZ() const {
    const float yawRad = yawDegrees * kDegToRad;
    return vmath::vec3(std::sin(yawRad), 0.0f, -std::cos(yawRad));
}

vmath::vec3 ThirdPersonCameraController::rightXZ() const {
    const float yawRad = yawDegrees * kDegToRad;
    return vmath::vec3(std::cos(yawRad), 0.0f, std::sin(yawRad));
}

bool ThirdPersonCameraController::isOrbitFollowing() const {
    return orbitFollowTimer > 0.0f;
}
