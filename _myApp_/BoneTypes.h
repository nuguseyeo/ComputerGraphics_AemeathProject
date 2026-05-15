#pragma once

#include <string>
#include <vmath.h>

// BoneData는 Assimp mesh bone을 렌더링용 스키닝 인덱스로 변환한 결과입니다.
struct BoneData {
    std::string name;
    int meshIndex = -1;
    int sourceIndex = -1;
    unsigned int weightCount = 0;
    vmath::mat4 offsetMatrix = vmath::mat4::identity();
    int animationIndex = -1;
    bool excludedFromLocomotion = false;
};

// BoneGroup은 이름 기반 휴리스틱으로 분류한 해부학적 부위입니다.
enum class BoneGroup {
    Pelvis,
    Waist,
    Clavicle,
    Shoulder,
    UpperArm,
    Elbow,
    Forearm,
    Wrist,
    Hand,
    Thigh,
    Knee,
    LowerLeg,
    Ankle,
    Foot,
    Unknown
};

// BoneSide는 좌/우/중앙 본을 구분해 보행 보정 부호를 다르게 적용하기 위한 값입니다.
enum class BoneSide {
    Center,
    Left,
    Right,
    Unknown
};

// BoneRole은 실제 본 이름이 모델마다 달라도 코드가 의미 단위로 접근하기 위한 역할 ID입니다.
enum class BoneRole {
    None,
    Root,
    Pelvis,
    Spine,
    Chest,
    LeftShoulder,
    RightShoulder,
    LeftUpperArm,
    RightUpperArm,
    LeftElbow,
    RightElbow,
    LeftHip,
    RightHip,
    LeftKnee,
    RightKnee
};