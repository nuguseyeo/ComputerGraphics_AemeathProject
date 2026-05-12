#include "AemeathApp.h"
#include <windows.h> 
#include <algorithm>
#include <cmath>
#include <cstdio>

static float NormalizeDegrees(float degrees) {
    degrees = std::fmod(degrees, 360.0f);
    if (degrees < 0.0f) {
        degrees += 360.0f;
    }
    return degrees;
}

static float DirectionToYawDegrees(float dirX, float dirZ) {
    return NormalizeDegrees(std::atan2(dirX, -dirZ) * 57.2957795f);
}

static float CharacterBackYawFromCamera(float cameraYawDegrees) {
    return NormalizeDegrees(180.0f - cameraYawDegrees);
}

static float CharacterFirstPersonYawFromCamera(float cameraYawDegrees) {
    return CharacterBackYawFromCamera(cameraYawDegrees);
}

void AemeathApp::startup() {
    AllocConsole();
    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONOUT$", "w", stderr);

    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
    cursorInfo.bVisible = false;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);

    char exePath[MAX_PATH] = {};
    GetModuleFileNameA(nullptr, exePath, MAX_PATH);
    char title[256] = {};
    std::snprintf(title, sizeof(title), "AemeathProject_Graphics_Ver1 | FirstPersonEyeNearer4 v2026-05-12 00:38");
    setWindowTitle(title);
    std::printf("[Runtime] %s\n", title);
    std::printf("[Runtime EXE] %s\n", exePath);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    humanModel.init("../forModel/Aemeath_HumanForm/Aemeath_source.pmx", "Model_vs.glsl", "Model_fs.glsl");
    mechaModel.init("../forModel/Aemeath_MechaForm/Aemeath_mecha_source.pmx", "Model_vs.glsl", "Model_fs.glsl");
}

void AemeathApp::render(double currentTime) {
    // 0. OpenGL 렌더링 영역을 현재 창 크기에 맞게 설정 (잘림 방지 핵심!)
    glViewport(0, 0, info.windowWidth, info.windowHeight);

    // 2. 화면 색상 및 깊이 버퍼 초기화 (중복 코드 제거하고 하나로 통합!)
    static const GLfloat dark_gray[] = { 0.1f, 0.1f, 0.1f, 1.0f };
    glClearBufferfv(GL_COLOR, 0, dark_gray);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // 깊이 버퍼 지우기 필수!

    // 깊이 테스트 켜기 (앞에 있는 물체가 뒤를 가리도록)
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    static double lastTime = 0.0;
    double deltaTime = currentTime - lastTime;
    lastTime = currentTime;
    if (deltaTime < 0.0) {
        deltaTime = 0.0;
    }
    if (deltaTime > 1.0 / 30.0) {
        deltaTime = 1.0 / 30.0;
    }

    // 현재 키보드 입력 상태 감지: 마우스로 정한 정면 방향을 기준으로 이동 벡터를 만든다.
    float localRight = 0.0f;
    float localForward = 0.0f;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) localForward += 1.0f;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) localForward -= 1.0f;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) localRight -= 1.0f;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) localRight += 1.0f;

    vmath::vec3 cameraForward = camera.forwardXZ();
    vmath::vec3 cameraRight = camera.rightXZ();
    float moveX = cameraRight[0] * localRight + cameraForward[0] * localForward;
    float moveZ = cameraRight[2] * localRight + cameraForward[2] * localForward;

    bool hasMovementInput = (localRight != 0.0f || localForward != 0.0f);
    bool isShiftHeld = (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS);

    // 달리기 상태 전환 로직: Shift를 꾹 누르고 있고 이동 중일 때만 달리기 유지
    if (isShiftHeld && hasMovementInput && !Aemeath.getIsDashing()) {
        Aemeath.setSprinting(true);
        isUIChanged = true;
    }
    else {
        // 방향키를 떼거나 Shift를 떼면 자동으로 걷기로 전환
        Aemeath.setSprinting(false);
    }

    // Moving State: WASD 입력 중에는 캐릭터의 등이 카메라를 향하도록 매 프레임 현재 카메라 yaw를 추적한다.
    if (hasMovementInput) {
        controlState = CharacterControlState::Moving;
        Aemeath.move(moveX, moveZ, deltaTime);
        isUIChanged = true;
    }
    else {
        controlState = CharacterControlState::Idle;
        // Idle State: 마우스 입력은 camera orbit만 바꾸며 캐릭터 model matrix는 유지된다.
    }

    if (viewMode == CameraViewMode::FirstPerson) {
        Aemeath.rotateToward(CharacterFirstPersonYawFromCamera(camera.yawDegrees), deltaTime);
    }
    else if (hasMovementInput || Aemeath.getIsDashing()) {
        Aemeath.rotateToward(CharacterBackYawFromCamera(camera.yawDegrees), deltaTime);
    }
    Aemeath.updatePhysics(deltaTime);
    if (Aemeath.getY() > 0.0f || Aemeath.getIsDashing()) {
        isUIChanged = true; // 공중에 있거나 대쉬 중일 땐 UI를 계속 갱신
    }

    if (isUIChanged) {
        UIManager::renderConsoleUI(Aemeath);
        isUIChanged = false;
    }

    // 모델의 종횡비 구하기 (창 너비 / 창 높이)
    float aspect = (float)info.windowWidth / (float)info.windowHeight;
    float cameraYaw = camera.yawDegrees;
    float modelFacingYaw = Aemeath.getYawDegrees();
    Model& activeModel = (Aemeath.getForm() == MECHA) ? mechaModel : humanModel;
    activeModel.draw((float)currentTime, aspect, vmath::vec3(Aemeath.getX(), Aemeath.getY(), Aemeath.getZ()), modelFacingYaw, cameraYaw, camera.pitchDegrees, camera.distance, viewMode == CameraViewMode::FirstPerson);
}

void AemeathApp::onKey(int key, int action) {
    if (key == GLFW_KEY_LEFT_ALT || key == GLFW_KEY_RIGHT_ALT) {
        if (action == GLFW_PRESS) {
            isCursorReleased = true;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
        else if (action == GLFW_RELEASE) {
            isCursorReleased = false;
            firstMouseMove = true;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
        return;
    }

    if (action == GLFW_PRESS) {
        switch (key) {
        case GLFW_KEY_E: Aemeath.useSkillE(); break;
        case GLFW_KEY_Q: Aemeath.useSkillQ(); break;
        case GLFW_KEY_R: Aemeath.useSkillR(); break;
        case GLFW_KEY_V:
            viewMode = (viewMode == CameraViewMode::ThirdPerson) ? CameraViewMode::FirstPerson : CameraViewMode::ThirdPerson;
            Aemeath.rotateToward(viewMode == CameraViewMode::FirstPerson ? CharacterFirstPersonYawFromCamera(camera.yawDegrees) : CharacterBackYawFromCamera(camera.yawDegrees), 1.0 / 60.0);
            firstMouseMove = true;
            break;
        case GLFW_KEY_SPACE: Aemeath.jump(); break;
        case GLFW_KEY_LEFT_SHIFT: {
            // Shift를 '누르는 순간' 마우스로 정한 정면 방향 기준으로 대쉬 발동
            float localRight = 0.0f;
            float localForward = 0.0f;
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) localForward += 1.0f;
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) localForward -= 1.0f;
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) localRight -= 1.0f;
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) localRight += 1.0f;

            vmath::vec3 cameraForward = camera.forwardXZ();
            vmath::vec3 cameraRight = camera.rightXZ();
            float dashX = cameraRight[0] * localRight + cameraForward[0] * localForward;
            float dashZ = cameraRight[2] * localRight + cameraForward[2] * localForward;
            if (localRight != 0.0f || localForward != 0.0f) {
                Aemeath.rotateToward(viewMode == CameraViewMode::FirstPerson ? CharacterFirstPersonYawFromCamera(camera.yawDegrees) : CharacterBackYawFromCamera(camera.yawDegrees), 1.0 / 60.0);
            }
            Aemeath.dash(dashX, dashZ);
            break;
        }
        case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose(window, true); break;
        }
        isUIChanged = true;
    }
}

void AemeathApp::onMouseButton(int button, int action) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        Aemeath.doNormalAttack();
        isUIChanged = true;
    }
}

void AemeathApp::onMouseMove(int x, int y) {
    if (isCursorReleased) return;

    if (firstMouseMove) {
        lastMouseX = x;
        lastMouseY = y;
        firstMouseMove = false;
        return;
    }

    int deltaX = x - lastMouseX;
    int deltaY = y - lastMouseY;
    lastMouseX = x;
    lastMouseY = y;

    const float mouseSensitivity = 0.06f;
    camera.applyMouseDelta(deltaX, deltaY, mouseSensitivity);
    if (viewMode == CameraViewMode::FirstPerson) {
        camera.pitchDegrees = std::max(-58.0f, std::min(55.0f, camera.pitchDegrees));
    }
    else if (camera.pitchDegrees < -20.0f) {
        camera.pitchDegrees = -20.0f;
    }
}

void AemeathApp::onMouseWheel(int pos) {
    camera.applyWheel(pos);
}

void AemeathApp::shutdown() {
    FreeConsole();
}
