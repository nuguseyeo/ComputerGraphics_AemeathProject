#pragma once

#include <vmath.h>

/// 위치, 회전, 스케일을 하나로 묶는 기본 변환 데이터.
/// 현재 프로젝트는 vmath를 사용하므로 glm 대신 vmath::vec3/mat4로 구성한다.
class Transform {
public:
    /// 월드 위치. x/y/z 축 기준 이동값이다.
    vmath::vec3 position = vmath::vec3(0.0f, 0.0f, 0.0f);
    /// 오일러 회전각(degrees). 현재 helper는 rotation[1]을 yaw로 사용한다.
    vmath::vec3 rotation = vmath::vec3(0.0f, 0.0f, 0.0f);
    /// 축별 스케일. 기본값은 원본 크기 유지다.
    vmath::vec3 scale = vmath::vec3(1.0f, 1.0f, 1.0f);

    /// translate * yaw * pitch * roll * scale 순서로 모델 행렬을 만든다.
    /// @return 렌더링 uniform "model"에 넘길 수 있는 월드 변환 행렬.
    vmath::mat4 getModelMatrix() const;

    /// yaw 기준 전방 벡터를 XZ 평면에서 계산한다.
    /// @return 정규화된 전방 방향. y는 항상 0이다.
    vmath::vec3 forward() const;

    /// yaw 기준 오른쪽 벡터를 XZ 평면에서 계산한다.
    /// @return 정규화된 오른쪽 방향. y는 항상 0이다.
    vmath::vec3 right() const;

    /// 월드 상방 벡터를 반환한다.
    /// @return 현재는 항상 (0, 1, 0).
    vmath::vec3 up() const;
};
