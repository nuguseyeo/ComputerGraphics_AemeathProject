#pragma once

#include <string>
#include "BoneTypes.h"

// BoneClassifier는 PMX/Assimp 본 이름을 기존 문자열 비교 규칙 그대로 역할/부위/좌우로 분류합니다.
// UTF-8 escape 문자열과 비교 순서는 Model.cpp에 있던 구현을 유지합니다.
namespace BoneClassifier {
const char* BoneGroupName(BoneGroup group);
const char* BoneSideName(BoneSide side);
std::string ToLowerCopy(std::string value);
std::string NormalizeBoneNameForRole(std::string value);
bool IsRelevantBoneName(const std::string& name);
bool IsFacialBoneName(const std::string& name);
bool IsExcludedFromPrimaryLocomotion(const std::string& name);
BoneRole ClassifyBoneRole(const std::string& boneName);
BoneSide ClassifyBoneSide(const std::string& name);
BoneGroup ClassifyBoneGroup(const std::string& name);
}