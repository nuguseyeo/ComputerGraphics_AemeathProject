#include "AemeathApp.h"
#include "GameConfig.h"
#include <windows.h> 
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

static std::ofstream gPmxDebugLog;

static std::string QuoteForPowerShellSingleQuotedString(const std::string& value) {
    std::string quoted;
    quoted.reserve(value.size() + 8);
    for (char c : value) {
        quoted += c;
        if (c == '\'') quoted += '\'';
    }
    return quoted;
}

static void StartPmxDebugLogWindow(const char* exePath) {
    char drive[_MAX_DRIVE] = {};
    char dir[_MAX_DIR] = {};
    _splitpath_s(exePath, drive, sizeof(drive), dir, sizeof(dir), nullptr, 0, nullptr, 0);
    std::string logPath = std::string(drive) + dir + "pmx_locomotion_debug.log";

    gPmxDebugLog.open(logPath, std::ios::out | std::ios::trunc);
    if (gPmxDebugLog.is_open()) {
        std::cout.rdbuf(gPmxDebugLog.rdbuf());
        std::cerr.rdbuf(gPmxDebugLog.rdbuf());
        std::cout << "============================================================\n";
        std::cout << "[LogWindow] PMX locomotion debug log started\n";
        std::cout << "[LogWindow] logPath=" << logPath << "\n";
        std::cout << "============================================================\n";
        std::cout.flush();
    }

    const std::string psPath = QuoteForPowerShellSingleQuotedString(logPath);
    std::string command = "powershell.exe -NoExit -Command \"$Host.UI.RawUI.WindowTitle='Aemeath PMX Locomotion Debug Log'; Get-Content -LiteralPath '" + psPath + "' -Encoding UTF8 -Wait\"";

    STARTUPINFOA startupInfo = {};
    PROCESS_INFORMATION processInfo = {};
    startupInfo.cb = sizeof(startupInfo);
    std::vector<char> mutableCommand(command.begin(), command.end());
    mutableCommand.push_back('\0');
    if (CreateProcessA(nullptr, mutableCommand.data(), nullptr, nullptr, FALSE, CREATE_NEW_CONSOLE, nullptr, nullptr, &startupInfo, &processInfo)) {
        CloseHandle(processInfo.hThread);
        CloseHandle(processInfo.hProcess);
    }
}

void AemeathApp::startup() {
    AllocConsole();
    SetConsoleTitleA("Aemeath Combat UI");
    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONOUT$", "w", stderr);

    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
    cursorInfo.bVisible = false;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);

    char exePath[MAX_PATH] = {};
    GetModuleFileNameA(nullptr, exePath, MAX_PATH);
    StartPmxDebugLogWindow(exePath);
    char title[256] = {};
    std::snprintf(title, sizeof(title), "AemeathProject_Graphics_Ver1 | PMXTextureStable v2026-05-13");
    setWindowTitle(title);
    std::printf("[Runtime] %s\n", title);
    std::printf("[Runtime EXE] %s\n", exePath);
    std::printf("============================================================\n");
    std::printf("[LogWindow] Separate PMX locomotion debug console initialized\n");
    std::printf("============================================================\n");

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    input.setWindow(window);
    humanModel.init("../forModel/Aemeath_HumanForm/Aemeath_source.pmx", "Model_skinning_vs.glsl", "Model_fs.glsl");
    mechaModel.init("../forModel/Aemeath_MechaForm/Aemeath_mecha_source.pmx", "Model_skinning_vs.glsl", "Model_fs.glsl");
}

void AemeathApp::render(double currentTime) {
    // Match the OpenGL viewport to the current window size.
    glViewport(0, 0, info.windowWidth, info.windowHeight);

    // Clear color and depth buffers.
    static const GLfloat dark_gray[] = { 0.1f, 0.1f, 0.1f, 1.0f };
    glClearBufferfv(GL_COLOR, 0, dark_gray);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Enable depth testing so nearer geometry occludes farther geometry.
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    const float deltaTime = updateDeltaTime(currentTime);
    const MovementInput movement = readMovementInput();
    updateCharacter(deltaTime, movement);
    updateConsoleUI();
    renderScene(currentTime, deltaTime, movement.hasMovement());
}

float AemeathApp::updateDeltaTime(double currentTime) {
    static double lastTime = 0.0;
    const RenderConfig config;
    double deltaTime = currentTime - lastTime;
    lastTime = currentTime;
    if (deltaTime < 0.0) {
        deltaTime = 0.0;
    }
    if (deltaTime > config.maxDeltaTime) {
        deltaTime = config.maxDeltaTime;
    }
    camera.updateOrbitFollow(static_cast<float>(deltaTime));
    return static_cast<float>(deltaTime);
}

MovementInput AemeathApp::readMovementInput() const {
    return characterController.buildMovementInput(
        input.isKeyDown(GLFW_KEY_W),
        input.isKeyDown(GLFW_KEY_S),
        input.isKeyDown(GLFW_KEY_A),
        input.isKeyDown(GLFW_KEY_D),
        camera);
}

void AemeathApp::updateCharacter(float deltaTime, const MovementInput& movement) {
    controlState = movement.hasMovement() ? CharacterControlState::Moving : CharacterControlState::Idle;
    if (characterController.update(Aemeath, movement, camera, viewMode, input.isKeyDown(GLFW_KEY_LEFT_SHIFT), deltaTime)) {
        isUIChanged = true;
    }
}

void AemeathApp::updateConsoleUI() {
    if (isUIChanged) {
        UIManager::renderConsoleUI(Aemeath);
        isUIChanged = false;
    }
}

void AemeathApp::renderScene(double currentTime, float deltaTime, bool hasMovementInput) {
    const RenderConfig config;
    const float aspect = static_cast<float>(info.windowWidth) / static_cast<float>(info.windowHeight);
    const float movementSpeedScale = (Aemeath.getIsSprinting() || Aemeath.getIsDashing())
        ? config.sprintAnimationSpeedScale
        : (hasMovementInput ? config.walkAnimationSpeedScale : 0.0f);
    Model& activeModel = (Aemeath.getForm() == MECHA) ? mechaModel : humanModel;
    activeModel.draw(
        static_cast<float>(currentTime),
        deltaTime,
        aspect,
        vmath::vec3(Aemeath.getX(), Aemeath.getY(), Aemeath.getZ()),
        Aemeath.getYawDegrees(),
        camera.yawDegrees,
        camera.pitchDegrees,
        camera.distance,
        viewMode == CameraViewMode::FirstPerson,
        hasMovementInput || Aemeath.getIsDashing(),
        movementSpeedScale);
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
        case GLFW_KEY_V: toggleViewMode(); break;
        case GLFW_KEY_SPACE: Aemeath.jump(); break;
        case GLFW_KEY_LEFT_SHIFT: handleDash(); break;
        case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose(window, true); break;
        default: handleDebugKey(key); break;
        }
        isUIChanged = true;
    }
}

void AemeathApp::toggleViewMode() {
    viewMode = (viewMode == CameraViewMode::ThirdPerson) ? CameraViewMode::FirstPerson : CameraViewMode::ThirdPerson;
    characterController.alignToCamera(Aemeath, camera, viewMode, 1.0f / 60.0f);
    firstMouseMove = true;
}

void AemeathApp::handleDash() {
    const MovementInput movement = readMovementInput();
    characterController.dash(Aemeath, movement, camera, viewMode);
}

void AemeathApp::handleDebugKey(int key) {
    switch (key) {
    case GLFW_KEY_1:
        humanModel.adjustShoulderCorrection(-5.0f);
        mechaModel.adjustShoulderCorrection(-5.0f);
        break;
    case GLFW_KEY_2:
        humanModel.adjustShoulderCorrection(5.0f);
        mechaModel.adjustShoulderCorrection(5.0f);
        break;
    case GLFW_KEY_3:
        humanModel.cycleShoulderCorrectionAxis();
        mechaModel.cycleShoulderCorrectionAxis();
        break;
    case GLFW_KEY_4:
        humanModel.flipLeftShoulderDownSign();
        mechaModel.flipLeftShoulderDownSign();
        break;
    case GLFW_KEY_5:
        humanModel.flipRightShoulderDownSign();
        mechaModel.flipRightShoulderDownSign();
        break;
    case GLFW_KEY_6:
        humanModel.printLocomotionDebug();
        mechaModel.printLocomotionDebug();
        break;
    case GLFW_KEY_7:
        humanModel.flipShoulderCorrectionSign();
        mechaModel.flipShoulderCorrectionSign();
        break;
    case GLFW_KEY_F1: {
        static bool bindPose = false;
        bindPose = !bindPose;
        humanModel.setBindPoseMode(bindPose);
        mechaModel.setBindPoseMode(bindPose);
        break;
    }
    case GLFW_KEY_F2: {
        static bool skeletonLines = false;
        skeletonLines = !skeletonLines;
        humanModel.setSkeletonDebugDraw(skeletonLines);
        mechaModel.setSkeletonDebugDraw(skeletonLines);
        break;
    }
    case GLFW_KEY_F3: {
        static bool singleBone = false;
        singleBone = !singleBone;
        humanModel.setSingleBoneTestMode(singleBone);
        mechaModel.setSingleBoneTestMode(singleBone);
        break;
    }
    case GLFW_KEY_F4:
        humanModel.cycleDebugJointTestMode();
        mechaModel.cycleDebugJointTestMode();
        break;
    case GLFW_KEY_F5:
        humanModel.cycleElbowBendAxis();
        mechaModel.cycleElbowBendAxis();
        break;
    case GLFW_KEY_F6:
        humanModel.adjustSingleBoneTestAngle(-5.0f);
        mechaModel.adjustSingleBoneTestAngle(-5.0f);
        break;
    case GLFW_KEY_F7:
        humanModel.adjustSingleBoneTestAngle(5.0f);
        mechaModel.adjustSingleBoneTestAngle(5.0f);
        break;
    case GLFW_KEY_F8:
        humanModel.printLocomotionDebug();
        mechaModel.printLocomotionDebug();
        break;
    case GLFW_KEY_F9:
        humanModel.cycleArmSwingAxis();
        mechaModel.cycleArmSwingAxis();
        break;
    case GLFW_KEY_F10:
        humanModel.cycleLegSwingAxis();
        mechaModel.cycleLegSwingAxis();
        break;
    case GLFW_KEY_F11:
        humanModel.cycleKneeBendAxis();
        mechaModel.cycleKneeBendAxis();
        break;
    case GLFW_KEY_F12: {
        const bool enabled = !humanModel.isProceduralLocomotionEnabled();
        humanModel.setProceduralLocomotionEnabled(enabled);
        mechaModel.setProceduralLocomotionEnabled(enabled);
        break;
    }
    default:
        break;
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

    camera.applyMouseDelta(deltaX, deltaY);
    if (deltaX != 0 || deltaY != 0) {
        camera.notifyOrbitInput();
    }
    camera.clampPitchForViewMode(viewMode);
}

void AemeathApp::onMouseWheel(int pos) {
    camera.applyWheel(pos);
}

void AemeathApp::shutdown() {
    FreeConsole();
}
