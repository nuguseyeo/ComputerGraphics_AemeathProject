#pragma once
#include <sb7.h>
#include <vmath.h>
#include <algorithm>
#include <cmath>
#include "Character.h"
#include "UIManager.h"
#include "Model.h"

enum class CharacterControlState {
    Idle,
    Moving
};

enum class CameraViewMode {
    ThirdPerson,
    FirstPerson
};

struct ThirdPersonCamera {
    float yawDegrees = 0.0f;
    float pitchDegrees = 14.0f;
    float distance = 28.6f;
    const float minDistance = 10.0f;
    const float maxDistance = 36.0f;
    const float minPitch = -86.4f;
    const float maxPitch = 39.0f;

    void applyMouseDelta(int deltaX, int deltaY, float sensitivity) {
        yawDegrees = std::fmod(yawDegrees + deltaX * sensitivity, 360.0f);
        if (yawDegrees < 0.0f) {
            yawDegrees += 360.0f;
        }
        pitchDegrees = std::max(minPitch, std::min(maxPitch, pitchDegrees - deltaY * sensitivity));
    }

    void applyWheel(int wheelDelta) {
        distance = std::max(minDistance, std::min(maxDistance, distance - wheelDelta * 0.65f));
    }

    vmath::vec3 forwardXZ() const {
        const float yawRad = yawDegrees * 0.0174532925f;
        return vmath::vec3(std::sin(yawRad), 0.0f, -std::cos(yawRad));
    }

    vmath::vec3 rightXZ() const {
        const float yawRad = yawDegrees * 0.0174532925f;
        return vmath::vec3(std::cos(yawRad), 0.0f, std::sin(yawRad));
    }
};
class AemeathApp : public sb7::application {
private:
    Character Aemeath;
    Model humanModel;
    Model mechaModel;
    bool isUIChanged = true;
    bool firstMouseMove = true;
    bool isCursorReleased = false;
    int lastMouseX = 0;
    int lastMouseY = 0;
    ThirdPersonCamera camera;
    CharacterControlState controlState = CharacterControlState::Idle;
    CameraViewMode viewMode = CameraViewMode::ThirdPerson;
    float cameraOrbitFollowTimer = 0.0f;

public:
    void startup() override;
    void render(double currentTime) override;
    void onKey(int key, int action) override;
    void onMouseButton(int button, int action) override;
    void onMouseMove(int x, int y) override;
    void onMouseWheel(int pos) override;
    void shutdown() override;
};
