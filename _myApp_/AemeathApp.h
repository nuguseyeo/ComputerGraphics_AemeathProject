#pragma once

#include <sb7.h>
#include "Character.h"
#include "CharacterController.h"
#include "Input.h"
#include "ThirdPersonCameraController.h"
#include "UIManager.h"
#include "Model.h"

// CharacterControlState는 현재 캐릭터가 입력에 의해 이동 중인지 나타내는 App 레벨 상태입니다.
enum class CharacterControlState {
    Idle,
    Moving
};

// AemeathApp은 sb7 프레임워크의 application 진입점입니다.
// 입력 해석, 캐릭터 제어, 카메라 제어, 모델 렌더링 호출을 전담 객체에 위임합니다.
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

    // humanModel과 mechaModel에 같은 디버그 명령을 적용합니다.
    // command는 Model& 하나를 받아 상태를 바꾸거나 로그를 출력하는 람다입니다.
    template <typename Command>
    void applyToBothModels(Command command) {
        command(humanModel);
        command(mechaModel);
    }

public:
    void startup() override;
    void render(double currentTime) override;
    void onKey(int key, int action) override;
    void onMouseButton(int button, int action) override;
    void onMouseMove(int x, int y) override;
    void onMouseWheel(int pos) override;
    void shutdown() override;
};