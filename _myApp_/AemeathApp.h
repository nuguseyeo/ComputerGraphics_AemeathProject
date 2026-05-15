#pragma once
#include <sb7.h>
#include "Character.h"
#include "CharacterController.h"
#include "Input.h"
#include "ThirdPersonCameraController.h"
#include "UIManager.h"
#include "Model.h"

enum class CharacterControlState {
    Idle,
    Moving
};

class AemeathApp : public sb7::application {
private:
    Character Aemeath;
    CharacterController characterController;
    Input input;
    Model humanModel;
    Model mechaModel;
    bool isUIChanged = true;
    bool firstMouseMove = true;
    bool isCursorReleased = false;
    int lastMouseX = 0;
    int lastMouseY = 0;
    ThirdPersonCameraController camera;
    CharacterControlState controlState = CharacterControlState::Idle;
    CameraViewMode viewMode = CameraViewMode::ThirdPerson;

    float updateDeltaTime(double currentTime);
    MovementInput readMovementInput() const;
    void updateCharacter(float deltaTime, const MovementInput& movement);
    void renderScene(double currentTime, float deltaTime, bool hasMovementInput);
    void updateConsoleUI();
    void toggleViewMode();
    void handleDebugKey(int key);
    void handleDash();

public:
    void startup() override;
    void render(double currentTime) override;
    void onKey(int key, int action) override;
    void onMouseButton(int button, int action) override;
    void onMouseMove(int x, int y) override;
    void onMouseWheel(int pos) override;
    void shutdown() override;
};
