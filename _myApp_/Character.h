#pragma once
#include "Common.h"
#include <string>

class Character {
private:
    // For Character Form
    CharacterForm form = HUMAN;

    // For Console output
    std::string lastActionMessage = "대기 중...";

    // For Skill
    int eStack = 0;
    const int MAX_E_STACK = 4;

    // 궁극기 활성화를 위한 게이지
    float ultGauge = 0.0f;
    const float MAX_ULT_GAUGE = 100.0f;

    // 2차 궁극기 사용을 위한 스택
    int ultStack = 0;
    const int MAX_ULT_STACK = 4;

    bool isUltActive = false;
    bool hasEnhancedAttack = false;

    // For Movement
    float posX = 0.0f, posY = 0.0f, posZ = 0.0f;
    float velocityY = 0.0f; // 점프 및 중력 연산을 위한 수직 속도
    bool isJumping = false; // 현재 공중에 있는지 확인하여 이중 점프를 방지
    bool isSprinting = false;
    float yawDegrees = 0.0f;

    // For Dash
    bool isDashing = false;
    float dashTimer = 0.0f;
    float dashDirX = 0.0f;
    float dashDirZ = 0.0f;

    const float GRAVITY = -15.0f;
    const float JUMP_POWER = 8.0f;
    const float MOVE_SPEED = 5.0f;
    const float SPRINT_MULTIPLIER = 2.0f;
    const float ROTATION_INTERPOLATION_SPEED = 8.0f;

    // 순간 이동 거리가 아닌, 대쉬 지속 시간과 속도로 변경
    const float DASH_DURATION = 0.25f; // 대쉬가 유지되는 시간 (0.25초)
    const float DASH_SPEED = 15.0f;    // 대쉬 중 이동 속도

public:
    // Getter 함수들
    CharacterForm getForm() const { return form; }
    std::string getLastMessage() const { return lastActionMessage; }
    int getEStack() const { return eStack; }
    int getMaxEStack() const { return MAX_E_STACK; }
    float getUltGauge() const { return ultGauge; }
    float getMaxUltGauge() const { return MAX_ULT_GAUGE; }
    int getUltStack() const { return ultStack; }
    int getMaxUltStack() const { return MAX_ULT_STACK; }
    bool getIsUltActive() const { return isUltActive; }
    bool getIsEnhanced() const { return hasEnhancedAttack; }

    float getX() const { return posX; }
    float getY() const { return posY; }
    float getZ() const { return posZ; }
    float getYawDegrees() const { return yawDegrees; }
    bool getIsSprinting() const { return isSprinting; }
    bool getIsDashing() const { return isDashing; } // UI 출력을 위한 Getter 추가

    // 액션 함수 (스킬)
    void doNormalAttack();
    void useSkillE();
    void useSkillQ();
    void useSkillR();

    // 이동 및 물리 로직 함수
    void move(float dirX, float dirZ, float deltaTime);
    void jump();
    void dash(float dirX, float dirZ);
    void setSprinting(bool state);
    void addYaw(float deltaDegrees);
    void setYaw(float degrees);
    void rotateToward(float targetDegrees, float deltaTime);
    void updatePhysics(float deltaTime);
};