#pragma once

#include <string>
#include <vector>
#include <vmath.h>
#include "BoneTypes.h"

// LocomotionState는 현재 절차적 보행 애니메이션의 큰 상태입니다.
enum class LocomotionState {
    Idle,
    Walk,
    Run
};

// LocomotionDebugLevel은 절차적 보행 적용 범위를 제한해 문제 부위를 좁히는 디버그 단계입니다.
enum class LocomotionDebugLevel {
    PelvisOnly,
    LeftLegOnly,
    LowerBodyOnly,
    FullBody
};

// DebugJointTestMode는 무릎/팔꿈치 축과 부호를 단일 관절 단위로 검증하기 위한 모드입니다.
enum class DebugJointTestMode {
    None,
    LeftKnee_X_Pos,
    LeftKnee_X_Neg,
    LeftKnee_Y_Pos,
    LeftKnee_Y_Neg,
    LeftKnee_Z_Pos,
    LeftKnee_Z_Neg,
    RightKnee_X_Pos,
    RightKnee_X_Neg,
    RightKnee_Y_Pos,
    RightKnee_Y_Neg,
    RightKnee_Z_Pos,
    RightKnee_Z_Neg,
    LeftElbow_X_Pos,
    LeftElbow_X_Neg,
    LeftElbow_Y_Pos,
    LeftElbow_Y_Neg,
    LeftElbow_Z_Pos,
    LeftElbow_Z_Neg,
    RightElbow_X_Pos,
    RightElbow_X_Neg,
    RightElbow_Y_Pos,
    RightElbow_Y_Neg,
    RightElbow_Z_Pos,
    RightElbow_Z_Neg
};

// LocalAxis는 본 로컬 공간 회전축을 지정합니다.
enum class LocalAxis {
    X,
    Y,
    Z
};

// JointBendConfig는 특정 관절이 어느 로컬 축으로 얼마나 굽는지 정의합니다.
struct JointBendConfig {
    LocalAxis axis = LocalAxis::X;
    float sign = 1.0f;
    float minAngleDeg = 0.0f;
    float maxAngleDeg = 30.0f;
};

// LimbBendConfig는 주요 사지 관절의 기본 굽힘 설정을 묶습니다.
struct LimbBendConfig {
    JointBendConfig leftKnee = { LocalAxis::X, 1.0f, 0.0f, 30.0f };
    JointBendConfig rightKnee = { LocalAxis::X, 1.0f, 0.0f, 30.0f };
    JointBendConfig leftElbow = { LocalAxis::X, -1.0f, 0.0f, 60.0f };
    JointBendConfig rightElbow = { LocalAxis::X, -1.0f, 0.0f, 60.0f };
};

// LocomotionBone은 모든 스키닝 본 중 절차적 보행 후보로 분류된 항목입니다.
struct LocomotionBone {
    int boneIndex = -1;
    BoneGroup group = BoneGroup::Unknown;
    BoneSide side = BoneSide::Unknown;
    bool appliesOffset = false;
};

// PrimaryLocomotionBone은 실제 보행 보정에 직접 사용하는 핵심 본입니다.
struct PrimaryLocomotionBone {
    std::string name;
    int skinningIndex = -1;
    BoneRole role = BoneRole::None;
    JointBendConfig bendOverride;
    bool hasBendOverride = false;
    float bendWeight = 1.0f;
    int bendStage = 0;
};

// SkeletonBoneMap은 자주 접근하는 주요 본 역할을 빠르게 찾기 위한 인덱스 캐시입니다.
struct SkeletonBoneMap {
    int root = -1;
    int pelvis = -1;
    int spine = -1;
    int chest = -1;
    int leftShoulder = -1;
    int rightShoulder = -1;
    int leftUpperArm = -1;
    int rightUpperArm = -1;
    int leftElbow = -1;
    int rightElbow = -1;
    int leftHip = -1;
    int rightHip = -1;
    int leftKnee = -1;
    int rightKnee = -1;
};

// ProceduralLocomotionSettings는 걷기/달리기 절차적 애니메이션의 튜닝값입니다.
struct ProceduralLocomotionSettings {
    float shoulderDownDeg = 35.0f;
    float walkFrequency = 6.0f;
    float walkArmSwingDeg = 30.0f;
    float walkLegSwingDeg = 30.0f;
    float walkKneeBendDeg = 30.0f;
    float walkElbowBaseBendDeg = 8.0f;
    float walkElbowSwingAddDeg = 22.0f;
    float walkPelvisYawDeg = 3.0f;
    float walkPelvisRollDeg = 2.0f;
    float runFrequency = 9.0f;
    float runArmSwingDeg = 35.0f;
    float runLegSwingDeg = 40.0f;
    float runKneeBendDeg = 60.0f;
    float runElbowBaseBendDeg = 25.0f;
    float runElbowSwingAddDeg = 35.0f;
    float runTorsoLeanDeg = 6.0f;
    float runPelvisYawDeg = 5.0f;
    float blendInSpeed = 8.0f;
    float blendOutSpeed = 6.0f;
    float runBlendSpeed = 5.0f;
    float runThreshold = 3.0f;
};

// LocomotionAxisSettings는 모델별 축/부호 차이를 런타임에 보정하기 위한 설정입니다.
struct LocomotionAxisSettings {
    int shoulderDownAxis = 2;
    int armSwingAxis = 0;
    int elbowBendAxis = 0;
    int legSwingAxis = 0;
    int kneeBendAxis = 0;
    int pelvisYawAxis = 1;
    int pelvisRollAxis = 2;
    int leftShoulderDownSign = 1;
    int rightShoulderDownSign = -1;
    int leftArmSwingSign = 1;
    int rightArmSwingSign = 1;
    int leftElbowSign = 1;
    int rightElbowSign = -1;
    int leftLegSign = 1;
    int rightLegSign = 1;
    int leftKneeSign = 1;
    int rightKneeSign = -1;
};