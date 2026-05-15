#include "Camera.h"
#include <cmath>

#pragma region Camera Constants
// 카메라 프레임 계산에 반복 사용되는 각도 변환 상수를 모은 구역입니다.

namespace {
constexpr float kDegToRad = 0.0174532925f;
constexpr float kFirstPersonFov = 32.0f;
constexpr float kThirdPersonFov = 45.0f;
constexpr float kFirstPersonNearPlane = 0.08f;
constexpr float kThirdPersonNearPlane = 0.1f;
constexpr float kFarPlane = 100.0f;
constexpr float kFallbackCameraTargetY = 8.0f;
constexpr float kEyeHeightOffset = 2.0f;
constexpr float kThirdPersonTargetHeightRatio = 0.72f;
constexpr float kFirstPersonEyeHeightRatio = 0.965f;
constexpr float kFirstPersonEyeForwardOffset = 0.16f;
constexpr float kMinimumEyeY = 0.5f;
constexpr float kFirstPersonMeshPitchCutoff = -6.0f;
constexpr float kFirstPersonChestClipRatio = 0.78f;
constexpr float kFallbackChestClipOffset = 1.35f;
}

#pragma endregion

#pragma region Camera Frame
// 카메라 상태와 캐릭터 위치를 view/projection/eye/target 프레임으로 변환하는 구역입니다.

CharacterCameraFrame Camera::buildCharacterFrame(
    const CameraState& camera,
    float aspect,
    const vmath::vec3& characterPosition,
    float modelMinY,
    float modelMaxY,
    bool hasModelBounds) {
    CharacterCameraFrame frame;

    const float yawRad = camera.yawDegrees * kDegToRad;
    const float pitchRad = camera.pitchDegrees * kDegToRad;
    const float horizontalDistance = std::cos(pitchRad) * camera.distance;
    frame.modelHeight = hasModelBounds ? (modelMaxY - modelMinY) : 0.0f;

    const float thirdPersonTargetY = hasModelBounds
        ? modelMinY + frame.modelHeight * kThirdPersonTargetHeightRatio
        : kFallbackCameraTargetY;
    frame.target = characterPosition + vmath::vec3(0.0f, thirdPersonTargetY, 0.0f);
    frame.eye = frame.target + vmath::vec3(
        -std::sin(yawRad) * horizontalDistance,
        std::sin(pitchRad) * camera.distance,
        std::cos(yawRad) * horizontalDistance);

    if (camera.firstPerson) {
        const float localEyeY = hasModelBounds
            ? modelMinY + frame.modelHeight * kFirstPersonEyeHeightRatio
            : kFallbackCameraTargetY;
        const float eyeHeight = localEyeY - kEyeHeightOffset;
        const vmath::vec3 horizontalView(std::sin(yawRad), 0.0f, -std::cos(yawRad));
        frame.eye = characterPosition + vmath::vec3(0.0f, eyeHeight, 0.0f) + horizontalView * kFirstPersonEyeForwardOffset;
        const vmath::vec3 lookDirection(
            std::sin(yawRad) * std::cos(pitchRad),
            std::sin(pitchRad),
            -std::cos(yawRad) * std::cos(pitchRad));
        frame.target = frame.eye + lookDirection;
    }

    if (frame.eye[1] < kMinimumEyeY) {
        frame.eye[1] = kMinimumEyeY;
    }

    frame.projection = vmath::perspective(
        camera.firstPerson ? kFirstPersonFov : kThirdPersonFov,
        aspect,
        camera.firstPerson ? kFirstPersonNearPlane : kThirdPersonNearPlane,
        kFarPlane);
    frame.view = vmath::lookat(frame.eye, frame.target, vmath::vec3(0.0f, 1.0f, 0.0f));
    frame.drawCharacterMesh = !camera.firstPerson || camera.pitchDegrees < kFirstPersonMeshPitchCutoff;
    frame.firstPersonChestClipY = hasModelBounds
        ? characterPosition[1] + modelMinY + frame.modelHeight * kFirstPersonChestClipRatio - kEyeHeightOffset
        : frame.eye[1] - kFallbackChestClipOffset;

    return frame;
}

#pragma endregion
