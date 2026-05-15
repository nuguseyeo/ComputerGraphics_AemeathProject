#include "BoneClassifier.h"

#include <algorithm>
#include <cctype>
#include <vector>

namespace BoneClassifier {

const char* BoneGroupName(BoneGroup group) {
    switch (group) {
    case BoneGroup::Pelvis: return "Pelvis";
    case BoneGroup::Waist: return "Waist";
    case BoneGroup::Clavicle: return "Clavicle";
    case BoneGroup::Shoulder: return "Shoulder";
    case BoneGroup::UpperArm: return "UpperArm";
    case BoneGroup::Elbow: return "Elbow";
    case BoneGroup::Forearm: return "Forearm";
    case BoneGroup::Wrist: return "Wrist";
    case BoneGroup::Hand: return "Hand";
    case BoneGroup::Thigh: return "Thigh";
    case BoneGroup::Knee: return "Knee";
    case BoneGroup::LowerLeg: return "LowerLeg";
    case BoneGroup::Ankle: return "Ankle";
    case BoneGroup::Foot: return "Foot";
    default: return "Unknown";
    }
}

const char* BoneSideName(BoneSide side) {
    switch (side) {
    case BoneSide::Center: return "Center";
    case BoneSide::Left: return "Left";
    case BoneSide::Right: return "Right";
    default: return "Unknown";
    }
}

std::string ToLowerCopy(std::string value) {
    for (char& c : value) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return value;
}

std::string NormalizeBoneNameForRole(std::string value) {
    value = ToLowerCopy(value);
    value.erase(std::remove_if(value.begin(), value.end(), [](char c) {
        return c == ' ' || c == '_' || c == '-' || c == '.';
    }), value.end());
    return value;
}

static bool ContainsAny(const std::string& value, const std::vector<std::string>& needles) {
    for (const std::string& needle : needles) {
        if (value.find(needle) != std::string::npos) {
            return true;
        }
    }
    return false;
}

bool IsRelevantBoneName(const std::string& name) {
    const std::string lower = ToLowerCopy(name);
    static const std::vector<std::string> asciiNeedles = {
        "shoulder", "clavicle",
        "upperarm", "upper_arm", "arm",
        "elbow", "forearm", "lowerarm", "lower_arm",
        "wrist", "hand",
        "hip", "pelvis", "waist",
        "thigh", "knee", "leg", "ankle"
    };
    if (ContainsAny(lower, asciiNeedles)) {
        return true;
    }

    static const std::vector<std::string> utf8Needles = {
        "\xE8\x82\xA9",
        "\xE8\x85\x95",
        "\xE8\x82\x98", "\xE3\x81\xB2\xE3\x81\x98",
        "\xE6\x89\x8B\xE9\xA6\x96",
        "\xE8\x85\xB0", "\xE9\xAA\xA8\xE7\x9B\xA4",
        "\xE4\xB8\x8B\xE5\x8D\x8A\xE8\xBA\xAB",
        "\xE8\xB6\xB3",
        "\xE8\x86\x9D", "\xE3\x81\xB2\xE3\x81\x96"
    };
    return ContainsAny(name, utf8Needles);
}

bool IsFacialBoneName(const std::string& name) {
    return ToLowerCopy(name).find("facial_") != std::string::npos;
}

bool IsExcludedFromPrimaryLocomotion(const std::string& name) {
    const std::string lower = ToLowerCopy(name);
    return lower.find("facial") != std::string::npos
        || lower.find("wrist") != std::string::npos
        || lower.find("hand") != std::string::npos
        || lower.find("finger") != std::string::npos
        || lower.find("thumb") != std::string::npos
        || lower.find("index") != std::string::npos
        || lower.find("middle") != std::string::npos
        || lower.find("ring") != std::string::npos
        || lower.find("pinky") != std::string::npos
        || lower.find("toe") != std::string::npos
        || lower.find("ankle") != std::string::npos
        || lower.find("foot") != std::string::npos
        || lower.find("hair") != std::string::npos
        || lower.find("skirt") != std::string::npos
        || lower.find("cloth") != std::string::npos
        || lower.find("ribbon") != std::string::npos
        || lower.find("weapon") != std::string::npos
        || lower.find("accessory") != std::string::npos
        || lower.find("bust") != std::string::npos
        || lower.find("chest") != std::string::npos
        || lower.find("ik") != std::string::npos
        || lower.find("roll") != std::string::npos
        || lower.find("twist") != std::string::npos
        || lower.find("target") != std::string::npos
        || lower.find("effector") != std::string::npos
        || lower.find("control") != std::string::npos
        || lower.find("display") != std::string::npos
        || lower.find("physics") != std::string::npos
        || lower.find("rigidbody") != std::string::npos
        || lower.find("jiggly") != std::string::npos
        || lower.find("spring") != std::string::npos
        || name.find("\xEF\xBC\xA9\xEF\xBC\xAB") != std::string::npos
        || name.find("\xE6\x8D\xA9") != std::string::npos
        || name.find("\xE8\xA6\xAA") != std::string::npos
        || name.find("\xE5\x85\x88") != std::string::npos
        || name.find("\xE8\xB6\xB3\xE9\xA6\x96") != std::string::npos
        || name.find("\xE6\x89\x8B\xE9\xA6\x96") != std::string::npos
        || name.find("\xE3\x81\xA4\xE3\x81\xBE\xE5\x85\x88") != std::string::npos
        || lower.find("dummy") != std::string::npos
        || lower.find("helper") != std::string::npos;
}

BoneRole ClassifyBoneRole(const std::string& boneName) {
    if (IsExcludedFromPrimaryLocomotion(boneName)) {
        return BoneRole::None;
    }

    const std::string normalized = NormalizeBoneNameForRole(boneName);
    if (boneName == "\xE3\x82\xBB\xE3\x83\xB3\xE3\x82\xBF\xE3\x83\xBC" || normalized == "center" || normalized == "root") {
        return BoneRole::Root;
    }
    if (boneName == "\xE4\xB8\x8B\xE5\x8D\x8A\xE8\xBA\xAB" || boneName == "\xE8\x85\xB0" || normalized == "pelvis" || normalized == "hip") {
        return BoneRole::Pelvis;
    }
    if (boneName.find("\xE4\xB8\x8A\xE5\x8D\x8A\xE8\xBA\xAB") != std::string::npos || normalized.find("spine") != std::string::npos) {
        return BoneRole::Spine;
    }
    if (normalized.find("chest") != std::string::npos) {
        return BoneRole::Chest;
    }
    const bool left = boneName.find("\xE5\xB7\xA6") != std::string::npos
        || normalized.rfind("left", 0) == 0 || normalized.rfind("l", 0) == 0 || normalized.size() > 1 && normalized.back() == 'l';
    const bool right = boneName.find("\xE5\x8F\xB3") != std::string::npos
        || normalized.rfind("right", 0) == 0 || normalized.rfind("r", 0) == 0 || normalized.size() > 1 && normalized.back() == 'r';

    if (!left && !right) {
        return BoneRole::None;
    }

    const bool shoulder = boneName.find("\xE8\x82\xA9") != std::string::npos
        || normalized.find("shoulder") != std::string::npos
        || normalized.find("clavicle") != std::string::npos;
    const bool upperArm = boneName == std::string(left ? "\xE5\xB7\xA6\xE8\x85\x95" : "\xE5\x8F\xB3\xE8\x85\x95")
        || normalized.find("upperarm") != std::string::npos
        || normalized.find("upperleg") == std::string::npos && normalized.find("arm001") != std::string::npos
        || normalized.find("upperleg") == std::string::npos && normalized.find("arm01") != std::string::npos;
    const bool elbow = boneName.find("\xE3\x81\xB2\xE3\x81\x98") != std::string::npos
        || boneName.find("\xE8\x82\x98") != std::string::npos
        || boneName.find("\xE3\x83\x92\xE3\x82\xB8") != std::string::npos
        || normalized.find("elbow") != std::string::npos;
    const std::string sideHipPrefix = left ? "\xE5\xB7\xA6\xE8\xB6\xB3" : "\xE5\x8F\xB3\xE8\xB6\xB3";
    const bool hip = boneName == sideHipPrefix
        || boneName == sideHipPrefix + "D"
        || normalized.find("thigh") != std::string::npos
        || normalized.find("upperleg") != std::string::npos
        || normalized.find("leg001") != std::string::npos
        || normalized.find("leg01") != std::string::npos
        || normalized.find("hip") != std::string::npos;
    const bool knee = boneName.find(left ? "\xE5\xB7\xA6\xE3\x81\xB2\xE3\x81\x96" : "\xE5\x8F\xB3\xE3\x81\xB2\xE3\x81\x96") != std::string::npos
        || boneName.find(left ? "\xE5\xB7\xA6\xE8\x86\x9D" : "\xE5\x8F\xB3\xE8\x86\x9D") != std::string::npos
        || boneName.find(left ? "\xE5\xB7\xA6\xE3\x83\x92\xE3\x82\xB6" : "\xE5\x8F\xB3\xE3\x83\x92\xE3\x82\xB6") != std::string::npos
        || normalized.find("knee") != std::string::npos;

    if (shoulder) return left ? BoneRole::LeftShoulder : BoneRole::RightShoulder;
    if (upperArm) return left ? BoneRole::LeftUpperArm : BoneRole::RightUpperArm;
    if (elbow) return left ? BoneRole::LeftElbow : BoneRole::RightElbow;
    if (hip) return left ? BoneRole::LeftHip : BoneRole::RightHip;
    if (knee) return left ? BoneRole::LeftKnee : BoneRole::RightKnee;
    return BoneRole::None;
}

BoneSide ClassifyBoneSide(const std::string& name) {
    const std::string lower = ToLowerCopy(name);
    if (lower.find("_l") != std::string::npos || name.find("\xE5\xB7\xA6") != std::string::npos) {
        return BoneSide::Left;
    }
    if (lower.find("_r") != std::string::npos || name.find("\xE5\x8F\xB3") != std::string::npos) {
        return BoneSide::Right;
    }
    return BoneSide::Center;
}

BoneGroup ClassifyBoneGroup(const std::string& name) {
    const std::string lower = ToLowerCopy(name);
    if (IsFacialBoneName(name)) return BoneGroup::Unknown;
    if (lower.find("pelvis") != std::string::npos || name.find("\xE9\xAA\xA8\xE7\x9B\xA4") != std::string::npos) return BoneGroup::Pelvis;
    if (lower.find("waist") != std::string::npos || lower.find("hip") != std::string::npos || name.find("\xE8\x85\xB0") != std::string::npos) return BoneGroup::Waist;
    if (lower.find("clavicle") != std::string::npos) return BoneGroup::Clavicle;
    if (lower.find("shoulder") != std::string::npos || name.find("\xE8\x82\xA9") != std::string::npos) return BoneGroup::Shoulder;
    if (lower.find("upperarm") != std::string::npos || lower.find("upper_arm") != std::string::npos || lower.find("arm") != std::string::npos || name.find("\xE8\x85\x95") != std::string::npos) return BoneGroup::UpperArm;
    if (lower.find("elbow") != std::string::npos || name.find("\xE8\x82\x98") != std::string::npos || name.find("\xE3\x81\xB2\xE3\x81\x98") != std::string::npos) return BoneGroup::Elbow;
    if (lower.find("forearm") != std::string::npos || lower.find("lowerarm") != std::string::npos || lower.find("lower_arm") != std::string::npos) return BoneGroup::Forearm;
    if (lower.find("wrist") != std::string::npos || name.find("\xE6\x89\x8B\xE9\xA6\x96") != std::string::npos) return BoneGroup::Wrist;
    if (lower.find("hand") != std::string::npos) return BoneGroup::Hand;
    if (lower.find("thigh") != std::string::npos) return BoneGroup::Thigh;
    if (lower.find("knee") != std::string::npos || name.find("\xE8\x86\x9D") != std::string::npos || name.find("\xE3\x81\xB2\xE3\x81\x96") != std::string::npos) return BoneGroup::Knee;
    if (lower.find("lowerleg") != std::string::npos || lower.find("lower_leg") != std::string::npos || lower.find("leg") != std::string::npos) return BoneGroup::LowerLeg;
    if (lower.find("ankle") != std::string::npos) return BoneGroup::Ankle;
    if (lower.find("foot") != std::string::npos || name.find("\xE8\xB6\xB3") != std::string::npos) return BoneGroup::Foot;
    return BoneGroup::Unknown;
}

}