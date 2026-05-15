#include "Character.h"
#include <cmath> // 벡터 정규화를 위해 추가
#include <algorithm>

#pragma region Skill Functions
// 캐릭터 스킬과 폼 전환처럼 전투 상태를 바꾸는 함수들을 모은 구역입니다.

void Character::doNormalAttack() {
    if (hasEnhancedAttack) {
        lastActionMessage = "[강화 평타] 레이저 발사! (버프 1회 소모)";
        hasEnhancedAttack = false;
    }
    else {
        if (eStack < MAX_E_STACK) {
            eStack++;
            lastActionMessage = "일반 공격 적중! (E스택 획득)";
        }
        else {
            lastActionMessage = "일반 공격! (E스택 최대치)";
        }
    }
    ultGauge = std::min(ultGauge + 5.0f, MAX_ULT_GAUGE);
}

void Character::useSkillE() {
    if (eStack < MAX_E_STACK) {
        form = (form == HUMAN) ? MECHA : HUMAN;
        lastActionMessage = std::string("E 스킬: 폼 체인지 -> ") + (form == HUMAN ? "인간형" : "로봇형");
    }
    else {
        eStack = 0;
        ultGauge = std::min(ultGauge + 10.0f, MAX_ULT_GAUGE);
        ultStack = std::min(ultStack + 1, MAX_ULT_STACK);
        lastActionMessage = "E 스킬: 스택 풀 차지 공격! 전방 강력 방출!";
    }
}

void Character::useSkillQ() {
    lastActionMessage = "Q 스킬: 반투명 로봇 낫 360도 회전!";
    ultGauge = std::min(ultGauge + 5.0f, MAX_ULT_GAUGE);
}

void Character::useSkillR() {
    if (ultGauge >= MAX_ULT_GAUGE || isUltActive) {
        if (!isUltActive) {
            form = (form == HUMAN) ? MECHA : HUMAN;
            isUltActive = true;
            hasEnhancedAttack = true;
            lastActionMessage = "*** 궁극기 1차 활성화! 폼 체인지 & 평타 강화! ***";
        }
        else {
            if (ultStack >= MAX_ULT_STACK) {
                ultGauge = 0.0f;
                ultStack = 0;
                isUltActive = false;
                hasEnhancedAttack = false;
                form = (form == HUMAN) ? MECHA : HUMAN;
                lastActionMessage = "*** 궁극기 2차 폭발! 모든 버프 해제 및 폼 체인지! ***";
            }
            else {
                lastActionMessage = "*** E 스택 부족! ***";
            }
        }
    }
    else {
        lastActionMessage = "시스템: 궁극기 게이지가 부족합니다.";
    }
}

#pragma endregion

#pragma region Movement Functions
// 이동, 점프, 대쉬, 회전, 물리 갱신처럼 캐릭터 위치/방향을 바꾸는 함수들을 모은 구역입니다.

void Character::move(float dirX, float dirZ, float deltaTime) {
    if (isDashing) return; // 대쉬 중에는 일반 이동 방향 입력을 무시하여 조작감을 높임

    // 대각선 이동 시 속도가 빨라지는 것을 막기 위한 정규화 (선택적)
    // 정확히 말하면 대각선으로 움직일때도 '1'만큼 움직이기 위해 정규화시키는 것!
    // 피타고라스 정리를 생각.
    float length = std::sqrt(dirX * dirX + dirZ * dirZ);    // 대각선 거리
    if (length > 0.0f) {    // Division By Zero 방지용 조건
        dirX /= length;
        dirZ /= length;
    }

    float currentSpeed = MOVE_SPEED * (isSprinting ? SPRINT_MULTIPLIER : 1.0f);
    posX += dirX * currentSpeed * deltaTime;
    posZ += dirZ * currentSpeed * deltaTime;
}

void Character::jump() {
    if (!isJumping) {
        isJumping = true;
        velocityY = JUMP_POWER;
        lastActionMessage = "스페이스바: 점프!";
    }
}

void Character::dash(float dirX, float dirZ) {
    if (isDashing) return; // 이미 대쉬 중이라면 무시

    if (dirX == 0.0f && dirZ == 0.0f) {
        const float yawRad = yawDegrees * 0.0174532925f;
        dirX = std::sin(yawRad);
        dirZ = -std::cos(yawRad);
    }

    // 대쉬 방향 벡터 정규화
    float length = std::sqrt(dirX * dirX + dirZ * dirZ);
    dashDirX = dirX / length;
    dashDirZ = dirZ / length;

    isDashing = true;
    dashTimer = DASH_DURATION; // 대쉬 지속 시간 설정
    lastActionMessage = "대쉬!";
}

void Character::setSprinting(bool state) {
    if (isSprinting != state) {
        isSprinting = state;
        if (!isDashing) {
            if (isSprinting) lastActionMessage = "상태: 달리기";
            else lastActionMessage = "상태: 걷기";
        }
    }
}

void Character::addYaw(float deltaDegrees) {
    setYaw(yawDegrees + deltaDegrees);
}

void Character::setYaw(float degrees) {
    yawDegrees = std::fmod(degrees, 360.0f);
    if (yawDegrees < 0.0f) {
        yawDegrees += 360.0f;
    }
}

void Character::rotateToward(float targetDegrees, float deltaTime) {
    targetDegrees = std::fmod(targetDegrees, 360.0f);
    if (targetDegrees < 0.0f) {
        targetDegrees += 360.0f;
    }

    float delta = targetDegrees - yawDegrees;
    while (delta > 180.0f) delta -= 360.0f;
    while (delta < -180.0f) delta += 360.0f;

    if (std::fabs(delta) < 0.05f) {
        setYaw(targetDegrees);
        return;
    }

    float blend = 1.0f - std::exp(-ROTATION_INTERPOLATION_SPEED * static_cast<float>(deltaTime));
    blend = std::max(0.0f, std::min(1.0f, blend));
    setYaw(yawDegrees + delta * blend);
}

void Character::updatePhysics(float deltaTime) {
    // 1. 대쉬 연산 (매 프레임마다 조금씩 이동)
    if (isDashing) {
        posX += dashDirX * DASH_SPEED * deltaTime;
        posZ += dashDirZ * DASH_SPEED * deltaTime;

        dashTimer -= deltaTime;
        if (dashTimer <= 0.0f) {
            isDashing = false;
            dashTimer = 0.0f;
            lastActionMessage = isSprinting ? "상태: 달리기" : "상태: 걷기";
        }
    }

    // 2. 중력 및 점프 연산
    if (isJumping) {
        posY += velocityY * deltaTime;
        velocityY += GRAVITY * deltaTime;

        if (posY <= 0.0f) {
            posY = 0.0f;
            isJumping = false;
            velocityY = 0.0f;
            lastActionMessage = "착지 완료";
        }
    }
}

#pragma endregion
