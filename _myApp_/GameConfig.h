#pragma once

struct CharacterMovementConfig {
    float dashFacingDeltaTime = 1.0f / 60.0f;
};

struct ThirdPersonCameraConfig {
    float initialPitchDegrees = 14.0f;
    float initialDistance = 28.6f;
    float minDistance = 10.0f;
    float maxDistance = 36.0f;
    float minPitch = -86.4f;
    float maxPitch = 39.0f;
    float firstPersonMinPitch = -58.0f;
    float firstPersonMaxPitch = 55.0f;
    float thirdPersonMinPitch = -20.0f;
    float mouseSensitivity = 0.035f;
    float wheelZoomStep = 0.65f;
    float orbitFollowDuration = 0.18f;
};

struct RenderConfig {
    float maxDeltaTime = 1.0f / 30.0f;
    float sprintAnimationSpeedScale = 2.0f;
    float walkAnimationSpeedScale = 1.0f;
};
