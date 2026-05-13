#include "AemeathApp.h"
#include <windows.h> 
#include <algorithm>
#include <cmath>
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
    std::string command = "powershell.exe -NoExit -Command \"$Host.UI.RawUI.WindowTitle='Aemeath PMX Locomotion Debug Log'; Get-Content -LiteralPath '" + psPath + "' -Wait\"";

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

static float CharacterYawFromMovementInput(float cameraYawDegrees, float localRight, float localForward, bool lateralFollowsCameraOrbit) {
    const float backYaw = CharacterBackYawFromCamera(cameraYawDegrees);

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

    static double lastTime = 0.0;
    double deltaTime = currentTime - lastTime;
    lastTime = currentTime;
    if (deltaTime < 0.0) {
        deltaTime = 0.0;
    }
    if (deltaTime > 1.0 / 30.0) {
        deltaTime = 1.0 / 30.0;
    }
    cameraOrbitFollowTimer = std::max(0.0f, cameraOrbitFollowTimer - static_cast<float>(deltaTime));

    // Build movement from keyboard input relative to the camera direction.
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

    // Keep sprinting only while Shift and movement input are active.
    if (isShiftHeld && hasMovementInput && !Aemeath.getIsDashing()) {
        Aemeath.setSprinting(true);
        isUIChanged = true;
    }
    else {
        // Return to walking when movement input or Shift is released.
        Aemeath.setSprinting(false);
    }

    // During movement, rotate the character back toward the camera yaw.
    if (hasMovementInput) {
        controlState = CharacterControlState::Moving;
        Aemeath.move(moveX, moveZ, deltaTime);
        isUIChanged = true;
    }
    else {
        controlState = CharacterControlState::Idle;
        // Mouse input changes only the camera orbit while idle.
    }

    if (viewMode == CameraViewMode::FirstPerson) {
        Aemeath.rotateToward(CharacterFirstPersonYawFromCamera(camera.yawDegrees), deltaTime);
    }
    else if (hasMovementInput || Aemeath.getIsDashing()) {
        const float targetYaw = hasMovementInput
            ? CharacterYawFromMovementInput(camera.yawDegrees, localRight, localForward, cameraOrbitFollowTimer > 0.0f)
            : CharacterBackYawFromCamera(camera.yawDegrees);
        Aemeath.rotateToward(targetYaw, deltaTime);
    }
    Aemeath.updatePhysics(deltaTime);
    if (Aemeath.getY() > 0.0f || Aemeath.getIsDashing()) {
        isUIChanged = true;
    }

    if (isUIChanged) {
        UIManager::renderConsoleUI(Aemeath);
        isUIChanged = false;
    }

    // Window aspect ratio for perspective projection.
    float aspect = (float)info.windowWidth / (float)info.windowHeight;
    float cameraYaw = camera.yawDegrees;
    float modelFacingYaw = Aemeath.getYawDegrees();
    Model& activeModel = (Aemeath.getForm() == MECHA) ? mechaModel : humanModel;
    const float movementSpeedScale = (Aemeath.getIsSprinting() || Aemeath.getIsDashing()) ? 2.0f : (hasMovementInput ? 1.0f : 0.0f);
    activeModel.draw((float)currentTime, static_cast<float>(deltaTime), aspect, vmath::vec3(Aemeath.getX(), Aemeath.getY(), Aemeath.getZ()), modelFacingYaw, cameraYaw, camera.pitchDegrees, camera.distance, viewMode == CameraViewMode::FirstPerson, hasMovementInput || Aemeath.getIsDashing(), movementSpeedScale);
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
        case GLFW_KEY_V:
            viewMode = (viewMode == CameraViewMode::ThirdPerson) ? CameraViewMode::FirstPerson : CameraViewMode::ThirdPerson;
            Aemeath.rotateToward(viewMode == CameraViewMode::FirstPerson ? CharacterFirstPersonYawFromCamera(camera.yawDegrees) : CharacterBackYawFromCamera(camera.yawDegrees), 1.0 / 60.0);
            firstMouseMove = true;
            break;
        case GLFW_KEY_SPACE: Aemeath.jump(); break;
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
        case GLFW_KEY_F4: {
            humanModel.cycleSingleBoneTestRole();
            mechaModel.cycleSingleBoneTestRole();
            break;
        }
        case GLFW_KEY_F5: {
            humanModel.cycleSingleBoneTestAxis();
            mechaModel.cycleSingleBoneTestAxis();
            break;
        }
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
        case GLFW_KEY_LEFT_SHIFT: {
            // Build movement from keyboard input relative to the camera direction.
            float localRight = 0.0f;
            float localForward = 0.0f;
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) localForward += 1.0f;
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) localForward -= 1.0f;
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) localRight -= 1.0f;
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) localRight += 1.0f;

            vmath::vec3 cameraForward = camera.forwardXZ();
            vmath::vec3 cameraRight = camera.rightXZ();
            // Dash in the current camera-relative movement direction.
            float dashX = cameraRight[0] * localRight + cameraForward[0] * localForward;
            float dashZ = cameraRight[2] * localRight + cameraForward[2] * localForward;
            if (localRight != 0.0f || localForward != 0.0f) {
                const float targetYaw = viewMode == CameraViewMode::FirstPerson
                    ? CharacterFirstPersonYawFromCamera(camera.yawDegrees)
                    : CharacterYawFromMovementInput(camera.yawDegrees, localRight, localForward, cameraOrbitFollowTimer > 0.0f);
                Aemeath.rotateToward(targetYaw, 1.0 / 60.0);
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

    const float mouseSensitivity = 0.035f;
    camera.applyMouseDelta(deltaX, deltaY, mouseSensitivity);
    if (deltaX != 0 || deltaY != 0) {
        cameraOrbitFollowTimer = 0.18f;
    }
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
