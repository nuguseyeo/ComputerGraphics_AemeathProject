#pragma once

/// CharacterController가 사용하는 캐릭터 이동 관련 튜닝 값 모음.
/// 현재는 대시를 시작할 때 캐릭터가 목표 방향을 바라보도록 보정하는 시간값만 포함한다.
struct CharacterMovementConfig {
    /// 대시 입력 순간 회전 보정에 사용할 고정 deltaTime.
    /// 프레임 입력 이벤트 안에서 호출되므로 실제 프레임 deltaTime 대신 안정적인 1/60초를 사용한다.
    float dashFacingDeltaTime = 1.0f / 60.0f;
};

/// ThirdPersonCameraController가 사용하는 orbit/zoom/pitch 설정값.
/// 값들은 기존 조작감을 보존하기 위해 리팩토링 전 하드코딩 값을 그대로 옮긴 것이다.
struct ThirdPersonCameraConfig {
    /// 게임 시작 시 카메라의 세로 각도. 양수는 위에서 내려다보는 방향에 가깝다.
    float initialPitchDegrees = 14.0f;
    /// 게임 시작 시 캐릭터와 카메라 사이 거리.
    float initialDistance = 28.6f;
    /// 휠 줌으로 가까워질 수 있는 최소 거리.
    float minDistance = 10.0f;
    /// 휠 줌으로 멀어질 수 있는 최대 거리.
    float maxDistance = 36.0f;
    /// 내부 pitch clamp의 최소값. 마우스 입력 후 먼저 이 범위로 제한된다.
    float minPitch = -86.4f;
    /// 내부 pitch clamp의 최대값. 마우스 입력 후 먼저 이 범위로 제한된다.
    float maxPitch = 39.0f;
    /// 1인칭 시점에서 허용하는 최소 pitch.
    float firstPersonMinPitch = -58.0f;
    /// 1인칭 시점에서 허용하는 최대 pitch.
    float firstPersonMaxPitch = 55.0f;
    /// 3인칭 시점에서 너무 낮게 내려가지 않도록 보정하는 pitch 하한.
    float thirdPersonMinPitch = -20.0f;
    /// 마우스 픽셀 이동량을 yaw/pitch 각도 변화량으로 변환하는 배율.
    float mouseSensitivity = 0.035f;
    /// 휠 입력 1단위가 카메라 거리에 반영되는 크기.
    float wheelZoomStep = 0.65f;
    /// 마우스로 orbit을 움직인 뒤 A/D 단독 이동 회전 보정을 다르게 처리하는 유지 시간.
    float orbitFollowDuration = 0.18f;
};

/// 프레임 루프와 렌더 호출에 필요한 공통 설정값.
struct RenderConfig {
    /// 긴 프레임 지연이 들어와도 물리/애니메이션이 튀지 않도록 deltaTime을 제한하는 최대값.
    float maxDeltaTime = 1.0f / 30.0f;
    /// 달리기나 대시 중 Model의 절차적 보행 애니메이션에 전달할 속도 배율.
    float sprintAnimationSpeedScale = 2.0f;
    /// 일반 이동 중 Model의 절차적 보행 애니메이션에 전달할 속도 배율.
    float walkAnimationSpeedScale = 1.0f;
};
