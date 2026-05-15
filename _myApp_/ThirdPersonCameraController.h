#pragma once

#include "Camera.h"
#include "GameConfig.h"
#include <vmath.h>

/// 마우스/휠 입력을 카메라 orbit 상태로 변환하는 컨트롤러.
/// 실제 view/projection 행렬은 Camera가 계산하고, 이 클래스는 입력 기반 상태만 관리한다.
class ThirdPersonCameraController {
public:
    /// 카메라 설정값을 받아 초기 pitch/distance를 구성한다.
    /// @param config 감도, 거리 제한, pitch 제한 등 카메라 튜닝값.
    explicit ThirdPersonCameraController(const ThirdPersonCameraConfig& config = ThirdPersonCameraConfig());

    /// 마우스 이동량을 yaw/pitch 변화로 적용한다.
    /// @param deltaX 이전 마우스 X와 현재 X의 차이. 양수면 yaw가 증가한다.
    /// @param deltaY 이전 마우스 Y와 현재 Y의 차이. 양수면 pitch가 감소한다.
    void applyMouseDelta(int deltaX, int deltaY);

    /// 휠 입력으로 카메라 거리를 조절한다.
    /// @param wheelDelta GLFW wheel callback에서 들어온 휠 변화량.
    void applyWheel(int wheelDelta);

    /// 현재 시점 모드에 맞춰 pitch를 추가 제한한다.
    /// @param viewMode 1인칭이면 1인칭 pitch 범위, 3인칭이면 3인칭 하한을 적용한다.
    void clampPitchForViewMode(CameraViewMode viewMode);

    /// orbit follow timer를 프레임 시간만큼 감소시킨다.
    /// @param deltaTime 초 단위 프레임 시간.
    void updateOrbitFollow(float deltaTime);

    /// 마우스 orbit 입력이 발생했음을 기록해 짧은 시간 동안 lateral 회전 규칙을 바꾼다.
    void notifyOrbitInput();

    /// 현재 yaw를 기준으로 XZ 평면 전방 벡터를 반환한다.
    vmath::vec3 forwardXZ() const;

    /// 현재 yaw를 기준으로 XZ 평면 오른쪽 벡터를 반환한다.
    vmath::vec3 rightXZ() const;

    /// 최근 orbit 입력 후 follow 유지 시간이 남아 있으면 true.
    bool isOrbitFollowing() const;

    /// 수평 회전각(degrees). App/ModelDrawParams로 전달된다.
    float yawDegrees = 0.0f;
    /// 수직 회전각(degrees). App/ModelDrawParams로 전달된다.
    float pitchDegrees;
    /// 3인칭 카메라 거리. App/ModelDrawParams로 전달된다.
    float distance;

private:
    /// 감도, 거리 제한, pitch 제한을 보관하는 설정값.
    ThirdPersonCameraConfig config;
    /// 마우스 orbit 직후 A/D 단독 이동 회전 보정에 사용하는 남은 시간.
    float orbitFollowTimer = 0.0f;
};
