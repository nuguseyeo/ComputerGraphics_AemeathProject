#pragma once

#include "GameConfig.h"
#include <vmath.h>

enum class CameraViewMode {
    ThirdPerson,
    FirstPerson
};

class ThirdPersonCameraController {
public:
    explicit ThirdPersonCameraController(const ThirdPersonCameraConfig& config = ThirdPersonCameraConfig());

    void applyMouseDelta(int deltaX, int deltaY);
    void applyWheel(int wheelDelta);
    void clampPitchForViewMode(CameraViewMode viewMode);
    void updateOrbitFollow(float deltaTime);
    void notifyOrbitInput();

    vmath::vec3 forwardXZ() const;
    vmath::vec3 rightXZ() const;
    bool isOrbitFollowing() const;

    float yawDegrees = 0.0f;
    float pitchDegrees;
    float distance;

private:
    ThirdPersonCameraConfig config;
    float orbitFollowTimer = 0.0f;
};
