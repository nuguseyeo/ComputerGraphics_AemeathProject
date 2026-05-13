#include "Model.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cmath>
#include <cstring>
#include <functional>
#include <windows.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#ifndef GL_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#endif
#ifndef GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#endif

static bool HasGLExtension(const char* extensionName) {
    const char* extensions = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
    return extensions && std::strstr(extensions, extensionName) != nullptr;
}

static void ApplyTextureQualitySettings() {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, -0.35f);

    if (HasGLExtension("GL_EXT_texture_filter_anisotropic")) {
        GLfloat maxAnisotropy = 1.0f;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, std::min(8.0f, maxAnisotropy));
    }
}

static std::string NormalizeTexturePath(const char* path) {
    std::string filename = path ? std::string(path) : std::string();

    for (char& c : filename) {
        if (c == '\\') c = '/';
    }

    while (!filename.empty() && std::isspace(static_cast<unsigned char>(filename.front()))) {
        filename.erase(filename.begin());
    }
    while (!filename.empty() && std::isspace(static_cast<unsigned char>(filename.back()))) {
        filename.pop_back();
    }
    if (filename.size() >= 2 && filename.front() == '"' && filename.back() == '"') {
        filename = filename.substr(1, filename.size() - 2);
    }

    return filename;
}

static bool ToWidePath(const std::string& src, UINT codePage, std::wstring& dst) {
    int len = MultiByteToWideChar(codePage, 0, src.c_str(), -1, nullptr, 0);
    if (len <= 0) return false;

    dst.assign(len - 1, L'\0');
    MultiByteToWideChar(codePage, 0, src.c_str(), -1, &dst[0], len);
    return true;
}

static FILE* OpenTextureFile(const std::string& filename) {
    std::wstring widePath;
    FILE* file = nullptr;

    if (ToWidePath(filename, CP_UTF8, widePath) && _wfopen_s(&file, widePath.c_str(), L"rb") == 0 && file) {
        return file;
    }
    if (ToWidePath(filename, CP_ACP, widePath) && _wfopen_s(&file, widePath.c_str(), L"rb") == 0 && file) {
        return file;
    }
    fopen_s(&file, filename.c_str(), "rb");
    return file;
}

static bool IsTextureVFlipTarget(const Texture& texture) {
    return false;
}

static vmath::mat4 ToVmathMatrix(const aiMatrix4x4& m) {
    return vmath::mat4(
        vmath::vec4(m.a1, m.b1, m.c1, m.d1),
        vmath::vec4(m.a2, m.b2, m.c2, m.d2),
        vmath::vec4(m.a3, m.b3, m.c3, m.d3),
        vmath::vec4(m.a4, m.b4, m.c4, m.d4)
    );
}

static const char* BoneGroupName(BoneGroup group) {
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

static const char* BoneSideName(BoneSide side) {
    switch (side) {
    case BoneSide::Center: return "Center";
    case BoneSide::Left: return "Left";
    case BoneSide::Right: return "Right";
    default: return "Unknown";
    }
}

static const char* LocomotionDebugLevelName(LocomotionDebugLevel level) {
    switch (level) {
    case LocomotionDebugLevel::PelvisOnly: return "PelvisOnly";
    case LocomotionDebugLevel::LeftLegOnly: return "LeftLegOnly";
    case LocomotionDebugLevel::LowerBodyOnly: return "LowerBodyOnly";
    case LocomotionDebugLevel::FullBody: return "FullBody";
    default: return "Unknown";
    }
}

static const char* BoneRoleName(BoneRole role) {
    switch (role) {
    case BoneRole::Root: return "Root";
    case BoneRole::Pelvis: return "Pelvis";
    case BoneRole::Spine: return "Spine";
    case BoneRole::Chest: return "Chest";
    case BoneRole::LeftShoulder: return "LeftShoulder";
    case BoneRole::RightShoulder: return "RightShoulder";
    case BoneRole::LeftUpperArm: return "LeftUpperArm";
    case BoneRole::RightUpperArm: return "RightUpperArm";
    case BoneRole::LeftElbow: return "LeftElbow";
    case BoneRole::RightElbow: return "RightElbow";
    case BoneRole::LeftHip: return "LeftHip";
    case BoneRole::RightHip: return "RightHip";
    case BoneRole::LeftKnee: return "LeftKnee";
    case BoneRole::RightKnee: return "RightKnee";
    default: return "None";
    }
}

static bool IsPrimaryTestRole(BoneRole role) {
    switch (role) {
    case BoneRole::LeftUpperArm:
    case BoneRole::RightUpperArm:
    case BoneRole::LeftElbow:
    case BoneRole::RightElbow:
    case BoneRole::LeftHip:
    case BoneRole::RightHip:
    case BoneRole::LeftKnee:
    case BoneRole::RightKnee:
        return true;
    default:
        return false;
    }
}

static const BoneRole kSingleBoneTestRoles[] = {
    BoneRole::LeftUpperArm,
    BoneRole::RightUpperArm,
    BoneRole::LeftElbow,
    BoneRole::RightElbow,
    BoneRole::LeftHip,
    BoneRole::RightHip,
    BoneRole::LeftKnee,
    BoneRole::RightKnee
};

static std::string ToLowerCopy(std::string value) {
    for (char& c : value) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return value;
}

static std::string NormalizeBoneNameForRole(std::string value) {
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

static bool IsRelevantBoneName(const std::string& name) {
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
        "\xE8\x82\xA9",                         // shoulder
        "\xE8\x85\x95",                         // arm
        "\xE8\x82\x98", "\xE3\x81\xB2\xE3\x81\x98", // elbow
        "\xE6\x89\x8B\xE9\xA6\x96",             // wrist
        "\xE8\x85\xB0", "\xE9\xAA\xA8\xE7\x9B\xA4", // waist / pelvis
        "\xE4\xB8\x8B\xE5\x8D\x8A\xE8\xBA\xAB", // lower body
        "\xE8\xB6\xB3",                         // leg / foot
        "\xE8\x86\x9D", "\xE3\x81\xB2\xE3\x81\x96"  // knee
    };
    return ContainsAny(name, utf8Needles);
}

static bool IsFacialBoneName(const std::string& name) {
    return ToLowerCopy(name).find("facial_") != std::string::npos;
}

static bool IsExcludedFromPrimaryLocomotion(const std::string& name) {
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
        || name.find("\xEF\xBC\xA9\xEF\xBC\xAB") != std::string::npos // full-width IK
        || name.find("\xE6\x8D\xA9") != std::string::npos             // twist
        || name.find("\xE8\xA6\xAA") != std::string::npos             // parent
        || name.find("\xE5\x85\x88") != std::string::npos             // tip
        || name.find("\xE6\x89\x8B\xE9\xA6\x96") != std::string::npos // wrist
        || name.find("\xE3\x81\xA4\xE3\x81\xBE\xE5\x85\x88") != std::string::npos // toe
        || lower.find("dummy") != std::string::npos
        || lower.find("helper") != std::string::npos;
}

static BoneRole ClassifyBoneRole(const std::string& boneName) {
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
        || normalized.find("elbow") != std::string::npos;
    const std::string sideHipPrefix = left ? "\xE5\xB7\xA6\xE8\xB6\xB3" : "\xE5\x8F\xB3\xE8\xB6\xB3";
    const bool hip = boneName == sideHipPrefix
        || boneName.rfind(sideHipPrefix, 0) == 0
        || normalized.find("thigh") != std::string::npos
        || normalized.find("upperleg") != std::string::npos
        || normalized.find("hip") != std::string::npos;
    const bool knee = boneName.find(left ? "\xE5\xB7\xA6\xE3\x81\xB2\xE3\x81\x96" : "\xE5\x8F\xB3\xE3\x81\xB2\xE3\x81\x96") != std::string::npos
        || boneName.find(left ? "\xE5\xB7\xA6\xE8\x86\x9D" : "\xE5\x8F\xB3\xE8\x86\x9D") != std::string::npos
        || normalized.find("knee") != std::string::npos;

    if (shoulder) return left ? BoneRole::LeftShoulder : BoneRole::RightShoulder;
    if (upperArm) return left ? BoneRole::LeftUpperArm : BoneRole::RightUpperArm;
    if (elbow) return left ? BoneRole::LeftElbow : BoneRole::RightElbow;
    if (hip) return left ? BoneRole::LeftHip : BoneRole::RightHip;
    if (knee) return left ? BoneRole::LeftKnee : BoneRole::RightKnee;
    return BoneRole::None;
}

static BoneSide ClassifyBoneSide(const std::string& name) {
    const std::string lower = ToLowerCopy(name);
    if (lower.find("_l") != std::string::npos || name.find("\xE5\xB7\xA6") != std::string::npos) {
        return BoneSide::Left;
    }
    if (lower.find("_r") != std::string::npos || name.find("\xE5\x8F\xB3") != std::string::npos) {
        return BoneSide::Right;
    }
    return BoneSide::Center;
}

static BoneGroup ClassifyBoneGroup(const std::string& name) {
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

static float Clamp01(float value) {
    return std::max(0.0f, std::min(1.0f, value));
}

static float Approach(float current, float target, float delta) {
    if (current < target) {
        return std::min(current + delta, target);
    }
    return std::max(current - delta, target);
}

static void AddBoneDataToVertex(Vertex& vertex, int boneID, float weight) {
    if (weight <= 0.0f || boneID < 0) return;
    for (int i = 0; i < 4; ++i) {
        if (vertex.boneIDs[i] < 0) {
            vertex.boneIDs[i] = boneID;
            vertex.boneWeights[i] = weight;
            return;
        }
    }

    int smallestIndex = 0;
    for (int i = 1; i < 4; ++i) {
        if (vertex.boneWeights[i] < vertex.boneWeights[smallestIndex]) {
            smallestIndex = i;
        }
    }
    if (weight > vertex.boneWeights[smallestIndex]) {
        vertex.boneIDs[smallestIndex] = boneID;
        vertex.boneWeights[smallestIndex] = weight;
    }
}

static float NormalizeVertexBoneWeights(Vertex& vertex) {
    float sum = 0.0f;
    for (int i = 0; i < 4; ++i) {
        if (vertex.boneIDs[i] >= 0 && vertex.boneWeights[i] > 0.0f) {
            sum += vertex.boneWeights[i];
        }
    }
    if (sum > 0.0f) {
        for (int i = 0; i < 4; ++i) {
            vertex.boneWeights[i] /= sum;
        }
    }
    return sum;
}

unsigned int TextureFromFile(const char* path, const std::string& directory) {
    std::string filename = NormalizeTexturePath(path);
    filename = directory + '/' + filename;

    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width = 0;
    int height = 0;
    int nrComponents = 0;

    // Assimp flips OBJ UVs once. Do not flip image rows again.
    stbi_set_flip_vertically_on_load(false);

    std::cout << "[Texture load] " << filename << std::endl;

    FILE* file = OpenTextureFile(filename);
    unsigned char* data = file ? stbi_load_from_file(file, &width, &height, &nrComponents, 0) : nullptr;
    if (file) fclose(file);

    if (data) {
        GLenum format = GL_RGB;
        GLenum internalFormat = GL_RGB8;
        if (nrComponents == 1) {
            format = GL_RED;
            internalFormat = GL_R8;
        }
        else if (nrComponents == 2) {
            format = GL_RG;
            internalFormat = GL_RG8;
        }
        else if (nrComponents == 3) {
            format = GL_RGB;
            internalFormat = GL_RGB8;
        }
        else if (nrComponents == 4) {
            format = GL_RGBA;
            internalFormat = GL_RGBA8;
        }
        else {
            std::cerr << " -> failed: unsupported channel count (" << nrComponents << ")" << std::endl;
            stbi_image_free(data);
            data = nullptr;
        }

        if (data) {
            glBindTexture(GL_TEXTURE_2D, textureID);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);

            ApplyTextureQualitySettings();

            stbi_image_free(data);
            std::cout << " -> success (" << width << "x" << height << ", channels=" << nrComponents << ")" << std::endl;
        }
    }
    if (!data) {
        std::cerr << " -> failed: " << filename << std::endl;

        glBindTexture(GL_TEXTURE_2D, textureID);
        unsigned char magenta[] = { 255, 0, 255, 255 };
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, magenta);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

    return textureID;
}

Mesh::Mesh(std::vector<Vertex> vertices, std::vector<GLuint> indices, std::vector<Texture> textures,
           vmath::vec3 diffuseColor, float opacity, bool flipTextureV, bool drawBackFacesFirst, bool alphaCutout) {
    this->vertices = vertices;
    this->indices = indices;
    this->textures = textures;
    this->diffuseColor = diffuseColor;
    this->opacity = opacity;
    this->flipTextureV = flipTextureV;
    this->drawBackFacesFirst = drawBackFacesFirst;
    this->alphaCutout = alphaCutout;
    this->hasDiffuseTexture = false;
    this->hasAlphaTexture = false;

    for (const Texture& texture : textures) {
        if (texture.type == "texture_diffuse") this->hasDiffuseTexture = true;
        if (texture.type == "texture_alpha") this->hasAlphaTexture = true;
    }

    setupMesh();
}

void Mesh::setupMesh() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), &indices[0], GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

    glEnableVertexAttribArray(3);
    glVertexAttribIPointer(3, 4, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, boneIDs));

    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, boneWeights));

    glBindVertexArray(0);
}

void Mesh::draw(GLuint shaderProgram) {
    glUniform3f(glGetUniformLocation(shaderProgram, "materialDiffuse"), diffuseColor[0], diffuseColor[1], diffuseColor[2]);
    glUniform1f(glGetUniformLocation(shaderProgram, "materialOpacity"), opacity);
    glUniform1i(glGetUniformLocation(shaderProgram, "useDiffuseTexture"), hasDiffuseTexture ? 1 : 0);
    glUniform1i(glGetUniformLocation(shaderProgram, "useAlphaTexture"), hasAlphaTexture ? 1 : 0);
    glUniform1i(glGetUniformLocation(shaderProgram, "flipTextureV"), flipTextureV ? 1 : 0);

    for (unsigned int i = 0; i < textures.size(); i++) {
        if (textures[i].type == "texture_diffuse") {
            glActiveTexture(GL_TEXTURE0);
            glUniform1i(glGetUniformLocation(shaderProgram, "texture_diffuse1"), 0);
            glBindTexture(GL_TEXTURE_2D, textures[i].id);
        }
        else if (textures[i].type == "texture_alpha") {
            glActiveTexture(GL_TEXTURE1);
            glUniform1i(glGetUniformLocation(shaderProgram, "texture_alpha1"), 1);
            glBindTexture(GL_TEXTURE_2D, textures[i].id);
        }
    }

    glBindVertexArray(VAO);
    if (drawBackFacesFirst) {
        GLboolean cullWasEnabled = glIsEnabled(GL_CULL_FACE);
        GLint previousCullFace = GL_BACK;
        glGetIntegerv(GL_CULL_FACE_MODE, &previousCullFace);

        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);

        glCullFace(GL_BACK);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);

        glCullFace(previousCullFace);
        if (!cullWasEnabled) {
            glDisable(GL_CULL_FACE);
        }
    }
    else {
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
    }
    glBindVertexArray(0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
}

bool Mesh::isTransparent() const {
    return !alphaCutout && (hasAlphaTexture || opacity < 0.999f);
}

void Model::init(const std::string& objFilePath, const std::string& vsPath, const std::string& fsPath) {
    GLuint vs = sb7::shader::load(vsPath.c_str(), GL_VERTEX_SHADER, true);
    GLuint fs = sb7::shader::load(fsPath.c_str(), GL_FRAGMENT_SHADER, true);

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vs);
    glAttachShader(shaderProgram, fs);
    glLinkProgram(shaderProgram);

    glDeleteShader(vs);
    glDeleteShader(fs);

    meshes.clear();
    textures_loaded.clear();
    relevantBones.clear();
    boneNameToIndex.clear();
    locomotionBones.clear();
    primaryLocomotionBones.clear();
    finalBoneMatrices.clear();
    rootNode = SkeletonNode();
    globalInverseTransform = vmath::mat4::identity();
    locomotionPhase = 0.0f;
    locomotionBlend = 0.0f;
    walkTime = 0.0f;
    walkBlend = 0.0f;
    locomotionState = LocomotionState::Idle;
    locomotionDebugLevel = LocomotionDebugLevel::FullBody;
    hasRequiredBoneRoles = false;
    bindPoseMode = false;
    skeletonDebugDraw = false;
    singleBoneTestMode = false;
    shoulderCorrectionDeg = locomotionSettings.shoulderDownDeg;
    shoulderCorrectionAxis = locomotionAxes.shoulderDownAxis;
    shoulderCorrectionSign = -1;
    singleBoneTestRole = BoneRole::LeftUpperArm;
    singleBoneTestAxis = 0;
    singleBoneTestAngleDeg = 30.0f;
    skinningVertexCount = 0;
    zeroWeightVertexCount = 0;
    badWeightVertexCount = 0;
    std::fill(std::begin(influenceHistogram), std::end(influenceHistogram), 0);
    minWeightSum = 9999.0f;
    maxWeightSum = 0.0f;
    totalWeightSum = 0.0;
    hasModelBounds = false;
    modelMinY = 0.0f;
    modelMaxY = 0.0f;

    loadModel(objFilePath);
    if (boneMatrixBuffer == 0) {
        glGenBuffers(1, &boneMatrixBuffer);
    }
    if (boneMatrixTexture == 0) {
        glGenTextures(1, &boneMatrixTexture);
    }
    setupFloor();
}

void Model::draw(float currentTime, float deltaTime, float aspect, const vmath::vec3& characterPosition, float characterYawDegrees, float cameraYawDegrees, float cameraPitchDegrees, float cameraDistance, bool firstPersonCamera, bool hasMovementInput, float movementSpeedScale) {
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glUseProgram(shaderProgram);
    updateLocomotion(deltaTime, hasMovementInput, movementSpeedScale);
    computeFinalBoneMatrices();

    const float degToRad = 0.0174532925f;
    const float yawRad = cameraYawDegrees * degToRad;
    const float pitchRad = cameraPitchDegrees * degToRad;
    const float horizontalDistance = std::cos(pitchRad) * cameraDistance;
    const float modelHeight = hasModelBounds ? (modelMaxY - modelMinY) : 0.0f;
    const float localEyeY = hasModelBounds ? (modelMinY + modelHeight * 0.965f) : 8.0f;
    const float eyeHeight = localEyeY - 2.0f;
    const float firstPersonEyeForwardOffset = 0.16f;

    const float thirdPersonTargetY = hasModelBounds
        ? modelMinY + modelHeight * 0.72f
        : 8.0f;
    vmath::vec3 target = characterPosition + vmath::vec3(0.0f, thirdPersonTargetY, 0.0f);
    vmath::vec3 eye = target + vmath::vec3(
        -std::sin(yawRad) * horizontalDistance,
        std::sin(pitchRad) * cameraDistance,
        std::cos(yawRad) * horizontalDistance
    );

    if (firstPersonCamera) {
        vmath::vec3 horizontalView(std::sin(yawRad), 0.0f, -std::cos(yawRad));
        eye = characterPosition + vmath::vec3(0.0f, eyeHeight, 0.0f) + horizontalView * firstPersonEyeForwardOffset;
        vmath::vec3 lookDirection(
            std::sin(yawRad) * std::cos(pitchRad),
            std::sin(pitchRad),
            -std::cos(yawRad) * std::cos(pitchRad)
        );
        target = eye + lookDirection;
    }

    if (eye[1] < 0.5f) {
        eye[1] = 0.5f;
    }

    vmath::mat4 projection = vmath::perspective(firstPersonCamera ? 32.0f : 45.0f, aspect, firstPersonCamera ? 0.08f : 0.1f, 100.0f);
    vmath::mat4 view = vmath::lookat(
        eye,
        target,
        vmath::vec3(0.0f, 1.0f, 0.0f)
    );
    vmath::mat4 model_matrix = vmath::translate(characterPosition[0], characterPosition[1] - 2.0f, characterPosition[2])
        * vmath::rotate(characterYawDegrees, 0.0f, 1.0f, 0.0f)
        * vmath::scale(1.0f);

    GLuint projLoc = glGetUniformLocation(shaderProgram, "projection");
    GLuint viewLoc = glGetUniformLocation(shaderProgram, "view");
    GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");

    glUniformMatrix4fv(projLoc, 1, GL_FALSE, projection);
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, view);
    glUniform3f(glGetUniformLocation(shaderProgram, "lightPos"), 18.0f, 28.0f, 12.0f);
    glUniform3f(glGetUniformLocation(shaderProgram, "viewPos"), eye[0], eye[1], eye[2]);
    glUniform3f(glGetUniformLocation(shaderProgram, "lightColor"), 1.0f, 0.94f, 0.82f);
    glUniform1f(glGetUniformLocation(shaderProgram, "ambientStrength"), 0.28f);
    glUniform1f(glGetUniformLocation(shaderProgram, "specularStrength"), 0.35f);
    glUniform1f(glGetUniformLocation(shaderProgram, "shininess"), 32.0f);

    glUniform1i(glGetUniformLocation(shaderProgram, "useFirstPersonClip"), 0);
    glUniform1i(glGetUniformLocation(shaderProgram, "useSkinning"), 0);
    drawFloor(shaderProgram);
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, model_matrix);
    uploadBoneMatrices(shaderProgram);
    const bool drawCharacterMesh = !firstPersonCamera || cameraPitchDegrees < -6.0f;
    glUniform1i(glGetUniformLocation(shaderProgram, "useFirstPersonClip"), firstPersonCamera ? 1 : 0);
    const float firstPersonChestClipY = hasModelBounds ? characterPosition[1] + modelMinY + modelHeight * 0.78f - 2.0f : eye[1] - 1.35f;
    glUniform1f(glGetUniformLocation(shaderProgram, "firstPersonClipMaxY"), firstPersonChestClipY);

    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);

    if (drawCharacterMesh) {
        drawSkeletonDebug(shaderProgram);
        for (unsigned int i = 0; i < meshes.size(); i++) {
            if (!meshes[i].isTransparent()) meshes[i].draw(shaderProgram);
        }
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    if (drawCharacterMesh) {
        for (unsigned int i = 0; i < meshes.size(); i++) {
            if (meshes[i].isTransparent()) meshes[i].draw(shaderProgram);
        }
    }
    glDepthMask(GL_TRUE);
}

void Model::loadModel(const std::string& path) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path,
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_CalcTangentSpace |
        aiProcess_FlipUVs);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
        return;
    }

    size_t pos = path.find_last_of("/\\");
    directory = (pos != std::string::npos) ? path.substr(0, pos) : ".";

    aiMatrix4x4 inverseRoot = scene->mRootNode->mTransformation;
    inverseRoot.Inverse();
    globalInverseTransform = ToVmathMatrix(inverseRoot);
    rootNode = copySkeletonNode(scene->mRootNode);
    processSkeletonData(scene);
    printFullSkeletonTable(scene);
    buildPrimaryLocomotionRig();
    processNode(scene->mRootNode, scene);
    printSkinningStats();
    setupSkeletonDebugLines();
}

SkeletonNode Model::copySkeletonNode(aiNode* node) const {
    SkeletonNode copied;
    if (!node) return copied;

    copied.name = node->mName.C_Str();
    copied.baseTransform = ToVmathMatrix(node->mTransformation);
    copied.children.reserve(node->mNumChildren);
    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
        copied.children.push_back(copySkeletonNode(node->mChildren[i]));
    }
    return copied;
}

void Model::collectSkeletonNodeNames(const SkeletonNode& node, std::vector<std::string>& names) const {
    if (!node.name.empty()) {
        names.push_back(node.name);
    }
    for (const SkeletonNode& child : node.children) {
        collectSkeletonNodeNames(child, names);
    }
}

const PrimaryLocomotionBone* Model::findPrimaryBoneByRole(BoneRole role) const {
    for (const PrimaryLocomotionBone& bone : primaryLocomotionBones) {
        if (bone.role == role) return &bone;
    }
    return nullptr;
}

const SkeletonNode* Model::findSkeletonNodeByName(const SkeletonNode& node, const std::string& name, const SkeletonNode** parent) const {
    if (node.name == name) return &node;
    for (const SkeletonNode& child : node.children) {
        if (child.name == name) {
            if (parent) *parent = &node;
            return &child;
        }
        const SkeletonNode* found = findSkeletonNodeByName(child, name, parent);
        if (found) return found;
    }
    return nullptr;
}

void Model::printSelectedBoneInfo(BoneRole role) const {
    const PrimaryLocomotionBone* primary = findPrimaryBoneByRole(role);
    if (!primary) {
        std::cout << "[SelectedBone] role=" << BoneRoleName(role) << " missing" << std::endl;
        return;
    }

    const SkeletonNode* parent = nullptr;
    const SkeletonNode* node = findSkeletonNodeByName(rootNode, primary->name, &parent);
    const bool excluded = IsExcludedFromPrimaryLocomotion(primary->name);
    const bool meshBone = primary->skinningIndex >= 0;
    unsigned int weightCount = 0;
    if (meshBone && primary->skinningIndex < static_cast<int>(relevantBones.size())) {
        weightCount = relevantBones[primary->skinningIndex].weightCount;
    }

    std::cout << "[SelectedBone] role=" << BoneRoleName(role)
        << ", name=" << primary->name
        << ", paletteIndex=" << primary->skinningIndex
        << ", aiMeshBone=" << (meshBone ? "yes" : "no")
        << ", weights=" << weightCount
        << ", excludedKeyword=" << (excluded ? "yes" : "no")
        << ", parent=" << (parent ? parent->name : "<none>")
        << ", children=" << (node ? node->children.size() : 0)
        << std::endl;

    if (node) {
        int descendantPaletteCount = 0;
        unsigned int descendantWeightCount = 0;
        std::vector<std::string> descendantPaletteNames;
        std::function<void(const SkeletonNode&)> collectDescendantPalette = [&](const SkeletonNode& current) {
            const auto foundPalette = boneNameToIndex.find(current.name);
            if (foundPalette != boneNameToIndex.end()) {
                const int paletteIndex = foundPalette->second;
                if (paletteIndex >= 0 && paletteIndex < static_cast<int>(relevantBones.size())) {
                    ++descendantPaletteCount;
                    descendantWeightCount += relevantBones[paletteIndex].weightCount;
                    if (descendantPaletteNames.size() < 12) {
                        descendantPaletteNames.push_back(current.name);
                    }
                }
            }
            for (const SkeletonNode& child : current.children) {
                collectDescendantPalette(child);
            }
        };
        collectDescendantPalette(*node);
        std::cout << "    descendantPaletteBones=" << descendantPaletteCount
            << ", descendantWeights=" << descendantWeightCount
            << std::endl;
        for (const std::string& descendantName : descendantPaletteNames) {
            std::cout << "    descendantPalette=" << descendantName << std::endl;
        }
        for (const SkeletonNode& child : node->children) {
            std::cout << "    child=" << child.name << std::endl;
        }
    }
}

void Model::printFullSkeletonTable(const aiScene* scene) const {
    if (!scene || !scene->mRootNode) return;

    struct MeshBoneInfo {
        unsigned int weightCount = 0;
        bool hasOffsetMatrix = false;
    };
    std::unordered_map<std::string, MeshBoneInfo> meshBoneInfo;
    for (unsigned int meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex) {
        aiMesh* mesh = scene->mMeshes[meshIndex];
        for (unsigned int boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex) {
            aiBone* bone = mesh->mBones[boneIndex];
            MeshBoneInfo& info = meshBoneInfo[bone->mName.C_Str()];
            info.weightCount += bone->mNumWeights;
            info.hasOffsetMatrix = true;
        }
    }

    std::cout << "[SkeletonTable] full node/bone table begin" << std::endl;
    std::function<void(aiNode*, const std::string&)> printNode = [&](aiNode* node, const std::string& parentName) {
        const std::string nodeName = node->mName.C_Str();
        const auto found = meshBoneInfo.find(nodeName);
        const bool isMeshBone = found != meshBoneInfo.end();
        const unsigned int weightCount = isMeshBone ? found->second.weightCount : 0;
        const bool hasOffset = isMeshBone && found->second.hasOffsetMatrix;
        std::cout << "  - name=" << nodeName
            << ", parent=" << (parentName.empty() ? "<none>" : parentName)
            << ", children=" << node->mNumChildren
            << ", aiMeshBone=" << (isMeshBone ? "yes" : "no")
            << ", weights=" << weightCount
            << ", offsetMatrix=" << (hasOffset ? "yes" : "no")
            << std::endl;
        for (unsigned int i = 0; i < node->mNumChildren; ++i) {
            printNode(node->mChildren[i], nodeName);
        }
    };
    printNode(scene->mRootNode, "");
    std::vector<std::string> nodeNames;
    collectSkeletonNodeNames(rootNode, nodeNames);
    for (const auto& entry : meshBoneInfo) {
        if (std::find(nodeNames.begin(), nodeNames.end(), entry.first) == nodeNames.end()) {
            std::cout << "  - name=" << entry.first
                << ", parent=<not in node tree>"
                << ", children=0"
                << ", aiMeshBone=yes"
                << ", weights=" << entry.second.weightCount
                << ", offsetMatrix=" << (entry.second.hasOffsetMatrix ? "yes" : "no")
                << std::endl;
        }
    }
    std::cout << "[SkeletonTable] full node/bone table end" << std::endl;
}

void Model::printSkinningStats() const {
    const double average = skinningVertexCount > 0 ? totalWeightSum / static_cast<double>(skinningVertexCount) : 0.0;
    std::cout << "[SkinningStats] vertices=" << skinningVertexCount
        << ", paletteBones=" << relevantBones.size()
        << ", weightSumMin=" << (skinningVertexCount > 0 ? minWeightSum : 0.0f)
        << ", weightSumMax=" << maxWeightSum
        << ", weightSumAvg=" << average
        << ", zeroWeightVertices=" << zeroWeightVertexCount
        << ", outside0.95to1.05=" << badWeightVertexCount
        << std::endl;
    std::cout << "[SkinningStats] influence histogram"
        << " 0=" << influenceHistogram[0]
        << " 1=" << influenceHistogram[1]
        << " 2=" << influenceHistogram[2]
        << " 3=" << influenceHistogram[3]
        << " 4+=" << influenceHistogram[4]
        << std::endl;
}

void Model::collectSkeletonDebugLines(const SkeletonNode& node, const vmath::mat4& parentTransform, std::vector<float>& lineVertices) const {
    const vmath::mat4 globalTransform = parentTransform * node.baseTransform;
    const vmath::vec3 parentPos(parentTransform[3][0], parentTransform[3][1], parentTransform[3][2]);
    const vmath::vec3 nodePos(globalTransform[3][0], globalTransform[3][1], globalTransform[3][2]);

    if (!node.name.empty()) {
        const float normalY = 1.0f;
        lineVertices.insert(lineVertices.end(), { parentPos[0], parentPos[1], parentPos[2], 0.0f, 0.0f, 0.0f, normalY, 0.0f });
        lineVertices.insert(lineVertices.end(), { nodePos[0], nodePos[1], nodePos[2], 0.0f, 0.0f, 0.0f, normalY, 0.0f });
    }

    for (const SkeletonNode& child : node.children) {
        collectSkeletonDebugLines(child, globalTransform, lineVertices);
    }
}

void Model::setupSkeletonDebugLines() {
    std::vector<float> lineVertices;
    if (!rootNode.name.empty()) {
        collectSkeletonDebugLines(rootNode, vmath::mat4::identity(), lineVertices);
    }
    skeletonLineVertexCount = static_cast<GLsizei>(lineVertices.size() / 8);
    if (skeletonLineVertexCount == 0) return;

    if (skeletonLineVAO == 0) glGenVertexArrays(1, &skeletonLineVAO);
    if (skeletonLineVBO == 0) glGenBuffers(1, &skeletonLineVBO);

    glBindVertexArray(skeletonLineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skeletonLineVBO);
    glBufferData(GL_ARRAY_BUFFER, lineVertices.size() * sizeof(float), lineVertices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
    glBindVertexArray(0);
}

void Model::drawSkeletonDebug(GLuint shaderProgram) {
    if (!skeletonDebugDraw || skeletonLineVAO == 0 || skeletonLineVertexCount == 0) return;

    glUniform1i(glGetUniformLocation(shaderProgram, "useSkinning"), 0);
    glUniform1i(glGetUniformLocation(shaderProgram, "useDiffuseTexture"), 0);
    glUniform1i(glGetUniformLocation(shaderProgram, "useAlphaTexture"), 0);
    glUniform1i(glGetUniformLocation(shaderProgram, "flipTextureV"), 0);
    glUniform3f(glGetUniformLocation(shaderProgram, "materialDiffuse"), 0.1f, 0.9f, 1.0f);
    glUniform1f(glGetUniformLocation(shaderProgram, "materialOpacity"), 1.0f);
    glLineWidth(2.0f);
    glBindVertexArray(skeletonLineVAO);
    glDrawArrays(GL_LINES, 0, skeletonLineVertexCount);
    glBindVertexArray(0);
    glLineWidth(1.0f);
}

void Model::buildPrimaryLocomotionRig() {
    primaryLocomotionBones.clear();
    boneMap = SkeletonBoneMap();
    hasRequiredBoneRoles = false;

    std::vector<std::string> candidateNames;
    collectSkeletonNodeNames(rootNode, candidateNames);
    for (const auto& entry : boneNameToIndex) {
        if (std::find(candidateNames.begin(), candidateNames.end(), entry.first) == candidateNames.end()) {
            candidateNames.push_back(entry.first);
        }
    }

    struct RoleCandidate {
        std::string name;
        int score = 0;
    };
    std::unordered_map<BoneRole, RoleCandidate> selectedNames;

    auto descendantPaletteStats = [&](const std::string& name, int& paletteCount, unsigned int& weightCount) {
        paletteCount = 0;
        weightCount = 0;
        const SkeletonNode* parent = nullptr;
        const SkeletonNode* node = findSkeletonNodeByName(rootNode, name, &parent);
        if (!node) return;
        std::function<void(const SkeletonNode&)> collect = [&](const SkeletonNode& current) {
            const auto foundPalette = boneNameToIndex.find(current.name);
            if (foundPalette != boneNameToIndex.end()) {
                const int paletteIndex = foundPalette->second;
                if (paletteIndex >= 0 && paletteIndex < static_cast<int>(relevantBones.size())) {
                    ++paletteCount;
                    weightCount += relevantBones[paletteIndex].weightCount;
                }
            }
            for (const SkeletonNode& child : current.children) {
                collect(child);
            }
        };
        collect(*node);
    };

    auto scoreName = [&](const std::string& name, BoneRole role) {
        int score = static_cast<int>(name.size());
        if (name.find("P") != std::string::npos || name.find("C") != std::string::npos || name.find("\xE8\xAA\xBF\xE6\x95\xB4") != std::string::npos) {
            score += 100;
        }
        const auto foundPalette = boneNameToIndex.find(name);
        if (foundPalette != boneNameToIndex.end()) {
            const int paletteIndex = foundPalette->second;
            if (paletteIndex >= 0 && paletteIndex < static_cast<int>(relevantBones.size()) && relevantBones[paletteIndex].weightCount > 0) {
                score -= 1000;
            }
        }

        int descendantPaletteCount = 0;
        unsigned int descendantWeightCount = 0;
        descendantPaletteStats(name, descendantPaletteCount, descendantWeightCount);
        if (descendantWeightCount > 0) {
            score -= 800;
        }
        else if (descendantPaletteCount > 0) {
            score -= 200;
        }

        if (role == BoneRole::LeftHip && name == "\xE5\xB7\xA6\xE8\xB6\xB3") score -= 300;
        if (role == BoneRole::RightHip && name == "\xE5\x8F\xB3\xE8\xB6\xB3") score -= 300;
        if (role == BoneRole::LeftKnee && (name == "\xE5\xB7\xA6\xE3\x81\xB2\xE3\x81\x96" || name == "\xE5\xB7\xA6\xE8\x86\x9D")) score -= 300;
        if (role == BoneRole::RightKnee && (name == "\xE5\x8F\xB3\xE3\x81\xB2\xE3\x81\x96" || name == "\xE5\x8F\xB3\xE8\x86\x9D")) score -= 300;
        return score;
    };

    for (const std::string& name : candidateNames) {
        const BoneRole role = ClassifyBoneRole(name);
        if (role == BoneRole::None) continue;
        const auto found = selectedNames.find(role);
        const int score = scoreName(name, role);
        if (found == selectedNames.end() || score < found->second.score) {
            selectedNames[role] = { name, score };
        }
    }

    auto addPrimary = [&](BoneRole role) {
        const auto foundName = selectedNames.find(role);
        if (foundName == selectedNames.end()) return;
        const auto foundSkinning = boneNameToIndex.find(foundName->second.name);
        PrimaryLocomotionBone bone;
        bone.name = foundName->second.name;
        bone.skinningIndex = foundSkinning != boneNameToIndex.end() ? foundSkinning->second : -1;
        bone.role = role;
        primaryLocomotionBones.push_back(bone);
    };

    addPrimary(BoneRole::Pelvis);
    addPrimary(BoneRole::Root);
    addPrimary(BoneRole::Spine);
    addPrimary(BoneRole::Chest);
    addPrimary(BoneRole::LeftShoulder);
    addPrimary(BoneRole::RightShoulder);
    addPrimary(BoneRole::LeftUpperArm);
    addPrimary(BoneRole::RightUpperArm);
    addPrimary(BoneRole::LeftElbow);
    addPrimary(BoneRole::RightElbow);
    addPrimary(BoneRole::LeftHip);
    addPrimary(BoneRole::RightHip);
    addPrimary(BoneRole::LeftKnee);
    addPrimary(BoneRole::RightKnee);

    for (size_t i = 0; i < primaryLocomotionBones.size(); ++i) {
        const PrimaryLocomotionBone& bone = primaryLocomotionBones[i];
        const int primaryIndex = static_cast<int>(i);
        switch (bone.role) {
        case BoneRole::Root: boneMap.root = primaryIndex; break;
        case BoneRole::Pelvis: boneMap.pelvis = primaryIndex; break;
        case BoneRole::Spine: boneMap.spine = primaryIndex; break;
        case BoneRole::Chest: boneMap.chest = primaryIndex; break;
        case BoneRole::LeftShoulder: boneMap.leftShoulder = primaryIndex; break;
        case BoneRole::RightShoulder: boneMap.rightShoulder = primaryIndex; break;
        case BoneRole::LeftUpperArm: boneMap.leftUpperArm = primaryIndex; break;
        case BoneRole::RightUpperArm: boneMap.rightUpperArm = primaryIndex; break;
        case BoneRole::LeftElbow: boneMap.leftElbow = primaryIndex; break;
        case BoneRole::RightElbow: boneMap.rightElbow = primaryIndex; break;
        case BoneRole::LeftHip: boneMap.leftHip = primaryIndex; break;
        case BoneRole::RightHip: boneMap.rightHip = primaryIndex; break;
        case BoneRole::LeftKnee: boneMap.leftKnee = primaryIndex; break;
        case BoneRole::RightKnee: boneMap.rightKnee = primaryIndex; break;
        default: break;
        }
    }

    hasRequiredBoneRoles = boneMap.leftUpperArm >= 0 && boneMap.rightUpperArm >= 0
        && boneMap.leftElbow >= 0 && boneMap.rightElbow >= 0
        && boneMap.leftHip >= 0 && boneMap.rightHip >= 0
        && boneMap.leftKnee >= 0 && boneMap.rightKnee >= 0;

    if (!hasRequiredBoneRoles) {
        std::cout << "[LocomotionPrimary] missing required 8 walk roles" << std::endl;
        for (BoneRole role : { BoneRole::LeftUpperArm, BoneRole::RightUpperArm, BoneRole::LeftElbow, BoneRole::RightElbow, BoneRole::LeftHip, BoneRole::RightHip, BoneRole::LeftKnee, BoneRole::RightKnee }) {
            std::cout << "  - " << BoneRoleName(role) << "=" << (selectedNames.count(role) ? selectedNames[role].name : "missing") << std::endl;
        }
    }

    std::cout << "[LocomotionPrimary] selected animated role bones: " << primaryLocomotionBones.size() << std::endl;
    for (const PrimaryLocomotionBone& bone : primaryLocomotionBones) {
        std::cout << "  - " << bone.name
            << " paletteIndex=" << bone.skinningIndex
            << " role=" << BoneRoleName(bone.role)
            << (bone.skinningIndex >= 0 ? "" : " (node-only hierarchy target)")
            << std::endl;
    }
}

void Model::processNode(aiNode* node, const aiScene* scene) {
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(processMesh(mesh, scene));
    }
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene);
    }
}

Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene) {
    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;
    std::vector<Texture> textures;

    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex;
        vertex.position = vmath::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
        if (!hasModelBounds) {
            modelMinY = vertex.position[1];
            modelMaxY = vertex.position[1];
            hasModelBounds = true;
        }
        else {
            modelMinY = std::min(modelMinY, vertex.position[1]);
            modelMaxY = std::max(modelMaxY, vertex.position[1]);
        }
        vertex.normal = mesh->HasNormals()
            ? vmath::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z)
            : vmath::vec3(0.0f, 0.0f, 0.0f);
        vertex.texCoord = mesh->mTextureCoords[0]
            ? vmath::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y)
            : vmath::vec2(0.0f, 0.0f);
        vertices.push_back(vertex);
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }

    for (unsigned int boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex) {
        aiBone* bone = mesh->mBones[boneIndex];
        const auto foundBone = boneNameToIndex.find(bone->mName.C_Str());
        if (foundBone == boneNameToIndex.end()) {
            continue;
        }

        const int animationBoneID = foundBone->second;
        for (unsigned int weightIndex = 0; weightIndex < bone->mNumWeights; ++weightIndex) {
            const aiVertexWeight& weight = bone->mWeights[weightIndex];
            if (weight.mVertexId < vertices.size()) {
                AddBoneDataToVertex(vertices[weight.mVertexId], animationBoneID, weight.mWeight);
            }
        }
    }

    for (Vertex& vertex : vertices) {
        const float originalWeightSum = NormalizeVertexBoneWeights(vertex);
        int influenceCount = 0;
        for (int i = 0; i < 4; ++i) {
            if (vertex.boneIDs[i] >= 0 && vertex.boneWeights[i] > 0.0f) {
                ++influenceCount;
            }
        }
        ++skinningVertexCount;
        influenceHistogram[std::min(influenceCount, 4)]++;
        minWeightSum = std::min(minWeightSum, originalWeightSum);
        maxWeightSum = std::max(maxWeightSum, originalWeightSum);
        totalWeightSum += originalWeightSum;
        if (originalWeightSum <= 0.0f) {
            ++zeroWeightVertexCount;
        }
        if (originalWeightSum < 0.95f || originalWeightSum > 1.05f) {
            ++badWeightVertexCount;
        }
    }

    vmath::vec3 diffuseColor = vmath::vec3(1.0f, 1.0f, 1.0f);
    float opacity = 1.0f;
    bool flipTextureV = false;
    bool drawBackFacesFirst = false;
    bool alphaCutout = false;

    if (mesh->mMaterialIndex >= 0) {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        aiColor3D color(1.0f, 1.0f, 1.0f);
        if (AI_SUCCESS == material->Get(AI_MATKEY_COLOR_DIFFUSE, color)) {
            diffuseColor = vmath::vec3(color.r, color.g, color.b);
        }
        material->Get(AI_MATKEY_OPACITY, opacity);

        std::vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

        std::vector<Texture> alphaMaps = loadMaterialTextures(material, aiTextureType_OPACITY, "texture_alpha");
        textures.insert(textures.end(), alphaMaps.begin(), alphaMaps.end());

        for (const Texture& texture : diffuseMaps) {
            if (texture.path.find("_Dal") != std::string::npos) {
                opacity = 0.5f;
                break;
            }
        }

        if (!alphaMaps.empty()) {
            opacity = 1.0f;
        }
        if (!alphaMaps.empty()) {
            for (const Texture& texture : textures) {
                if (IsTextureVFlipTarget(texture)) {
                    flipTextureV = true;
                    drawBackFacesFirst = true;
                    break;
                }
            }
        }
    }

    return Mesh(vertices, indices, textures, diffuseColor, opacity, flipTextureV, drawBackFacesFirst, alphaCutout);
}

void Model::processSkeletonData(const aiScene* scene) {
    if (!scene) return;

    for (unsigned int meshIndex = 0; meshIndex < scene->mNumMeshes; meshIndex++) {
        aiMesh* mesh = scene->mMeshes[meshIndex];
        for (unsigned int boneIndex = 0; boneIndex < mesh->mNumBones; boneIndex++) {
            aiBone* bone = mesh->mBones[boneIndex];
            const std::string boneName = bone->mName.C_Str();

            bool alreadyLoaded = false;
            for (const BoneData& loadedBone : relevantBones) {
                if (loadedBone.name == boneName) {
                    alreadyLoaded = true;
                    break;
                }
            }
            if (alreadyLoaded) {
                continue;
            }

            BoneData boneData;
            boneData.name = boneName;
            boneData.meshIndex = static_cast<int>(meshIndex);
            boneData.sourceIndex = static_cast<int>(boneIndex);
            boneData.weightCount = bone->mNumWeights;
            boneData.offsetMatrix = ToVmathMatrix(bone->mOffsetMatrix);
            boneData.animationIndex = static_cast<int>(relevantBones.size());
            boneData.excludedFromLocomotion = IsExcludedFromPrimaryLocomotion(boneName);
            boneNameToIndex[boneName] = boneData.animationIndex;
            relevantBones.push_back(boneData);

            LocomotionBone locomotionBone;
            locomotionBone.boneIndex = boneData.animationIndex;
            locomotionBone.group = ClassifyBoneGroup(boneName);
            locomotionBone.side = ClassifyBoneSide(boneName);
            locomotionBone.appliesOffset = false;
            locomotionBones.push_back(locomotionBone);
        }
    }

    finalBoneMatrices.assign(relevantBones.size(), vmath::mat4::identity());
    std::cout << "[Skeleton] aiMesh bones loaded for skinning palette: " << relevantBones.size() << std::endl;
    for (const BoneData& bone : relevantBones) {
        const LocomotionBone& locomotionBone = locomotionBones[bone.animationIndex];
        std::cout << "  - " << bone.name << " (mesh=" << bone.meshIndex
            << ", weights=" << bone.weightCount
            << ", group=" << BoneGroupName(locomotionBone.group)
            << ", side=" << BoneSideName(locomotionBone.side)
            << ", facialExcluded=" << (bone.excludedFromLocomotion ? "yes" : "no")
            << ", animated=" << (locomotionBone.appliesOffset ? "yes" : "no")
            << ")" << std::endl;
    }
}

void Model::setProceduralLocomotionEnabled(bool enabled) {
    proceduralLocomotionEnabled = enabled;
    std::cout << "[Locomotion] procedural locomotion " << (enabled ? "enabled" : "disabled") << std::endl;
}

void Model::setLocomotionDebugLevel(LocomotionDebugLevel level) {
    locomotionDebugLevel = level;
    std::cout << "[Locomotion] debug level=" << LocomotionDebugLevelName(level) << std::endl;
}

void Model::setBindPoseMode(bool enabled) {
    bindPoseMode = enabled;
    std::cout << "[Locomotion] bind pose mode " << (enabled ? "enabled" : "disabled") << std::endl;
}

void Model::setSkeletonDebugDraw(bool enabled) {
    skeletonDebugDraw = enabled;
    std::cout << "[SkeletonDebug] line draw " << (enabled ? "enabled" : "disabled") << " (console mode only)" << std::endl;
}

void Model::setSingleBoneTestMode(bool enabled) {
    singleBoneTestMode = enabled;
    std::cout << "[Locomotion] single bone test " << (enabled ? "enabled" : "disabled") << std::endl;
}

void Model::adjustShoulderCorrection(float deltaDegrees) {
    shoulderCorrectionDeg = std::max(0.0f, std::min(100.0f, shoulderCorrectionDeg + deltaDegrees));
    locomotionSettings.shoulderDownDeg = shoulderCorrectionDeg;
    printLocomotionDebug();
}

void Model::cycleShoulderCorrectionAxis() {
    locomotionAxes.shoulderDownAxis = (locomotionAxes.shoulderDownAxis + 1) % 3;
    shoulderCorrectionAxis = locomotionAxes.shoulderDownAxis;
    printLocomotionDebug();
}

void Model::cycleArmSwingAxis() {
    locomotionAxes.armSwingAxis = (locomotionAxes.armSwingAxis + 1) % 3;
    printLocomotionDebug();
}

void Model::cycleLegSwingAxis() {
    locomotionAxes.legSwingAxis = (locomotionAxes.legSwingAxis + 1) % 3;
    printLocomotionDebug();
}

void Model::cycleKneeBendAxis() {
    locomotionAxes.kneeBendAxis = (locomotionAxes.kneeBendAxis + 1) % 3;
    printLocomotionDebug();
}

void Model::flipShoulderCorrectionSign() {
    shoulderCorrectionSign *= -1;
    printLocomotionDebug();
}

void Model::flipLeftShoulderDownSign() {
    locomotionAxes.leftShoulderDownSign *= -1;
    printLocomotionDebug();
}

void Model::flipRightShoulderDownSign() {
    locomotionAxes.rightShoulderDownSign *= -1;
    printLocomotionDebug();
}

void Model::cycleSingleBoneTestRole() {
    const int roleCount = static_cast<int>(sizeof(kSingleBoneTestRoles) / sizeof(kSingleBoneTestRoles[0]));
    int currentIndex = 0;
    for (int i = 0; i < roleCount; ++i) {
        if (kSingleBoneTestRoles[i] == singleBoneTestRole) {
            currentIndex = i;
            break;
        }
    }
    singleBoneTestRole = kSingleBoneTestRoles[(currentIndex + 1) % roleCount];
    printSelectedBoneInfo(singleBoneTestRole);
    printLocomotionDebug();
}

void Model::cycleSingleBoneTestAxis() {
    singleBoneTestAxis = (singleBoneTestAxis + 1) % 3;
    printSelectedBoneInfo(singleBoneTestRole);
    printLocomotionDebug();
}

void Model::adjustSingleBoneTestAngle(float deltaDegrees) {
    singleBoneTestAngleDeg = std::max(-45.0f, std::min(45.0f, singleBoneTestAngleDeg + deltaDegrees));
    printSelectedBoneInfo(singleBoneTestRole);
    printLocomotionDebug();
}

void Model::setRunModeEnabled(bool enabled) {
    forceRunAnimation = enabled;
    std::cout << "[Locomotion] forced run animation " << (enabled ? "enabled" : "disabled") << std::endl;
}

void Model::printLocomotionDebug() const {
    std::cout << "\n============================================================" << std::endl;
    std::cout << "[LocomotionDebug] snapshot begin" << std::endl;
    std::cout << "============================================================" << std::endl;
    std::cout << "[Locomotion] enabled=" << (proceduralLocomotionEnabled ? "yes" : "no")
        << ", phase=" << locomotionPhase
        << ", blend=" << walkBlend
        << ", runBlend=" << runBlend
        << ", debugLevel=" << LocomotionDebugLevelName(locomotionDebugLevel)
        << ", bindPose=" << (bindPoseMode ? "yes" : "no")
        << ", skeletonLines=" << (skeletonDebugDraw ? "yes" : "no")
        << ", singleBoneTest=" << (singleBoneTestMode ? "yes" : "no")
        << ", shoulderCorrectionDeg=" << shoulderCorrectionDeg
        << ", shoulderDownAxis=" << locomotionAxes.shoulderDownAxis
        << ", armSwingAxis=" << locomotionAxes.armSwingAxis
        << ", elbowBendAxis=" << locomotionAxes.elbowBendAxis
        << ", legSwingAxis=" << locomotionAxes.legSwingAxis
        << ", kneeBendAxis=" << locomotionAxes.kneeBendAxis
        << ", shoulderCorrectionSign=" << shoulderCorrectionSign
        << ", leftShoulderDownSign=" << locomotionAxes.leftShoulderDownSign
        << ", rightShoulderDownSign=" << locomotionAxes.rightShoulderDownSign
        << ", singleTestRole=" << BoneRoleName(singleBoneTestRole)
        << ", singleTestAxis=" << singleBoneTestAxis
        << ", singleTestAngle=" << singleBoneTestAngleDeg
        << ", shaderBones=" << locomotionBones.size()
        << ", primaryBones=" << primaryLocomotionBones.size() << std::endl;
    const float currentArmSwingDeg = locomotionSettings.walkArmSwingDeg + (locomotionSettings.runArmSwingDeg - locomotionSettings.walkArmSwingDeg) * runBlend;
    const float currentLegSwingDeg = locomotionSettings.walkLegSwingDeg + (locomotionSettings.runLegSwingDeg - locomotionSettings.walkLegSwingDeg) * runBlend;
    const float rightArm = std::sin(locomotionPhase) * currentArmSwingDeg;
    const float leftArm = -rightArm;
    const float leftLeg = std::sin(locomotionPhase) * currentLegSwingDeg;
    const float rightLeg = -leftLeg;
    std::cout << "[LocomotionPhase] rightArm=" << rightArm
        << ", leftArm=" << leftArm
        << ", leftLeg=" << leftLeg
        << ", rightLeg=" << rightLeg
        << ", rightArm+leftArm=" << (rightArm + leftArm)
        << ", leftLeg-rightArm=" << (leftLeg - rightArm)
        << ", rightLeg-leftArm=" << (rightLeg - leftArm)
        << std::endl;
    printSelectedBoneInfo(BoneRole::LeftUpperArm);
    printSelectedBoneInfo(BoneRole::RightUpperArm);
    printSelectedBoneInfo(BoneRole::LeftElbow);
    printSelectedBoneInfo(BoneRole::RightElbow);
    printSelectedBoneInfo(BoneRole::LeftHip);
    printSelectedBoneInfo(BoneRole::RightHip);
    printSelectedBoneInfo(BoneRole::LeftKnee);
    printSelectedBoneInfo(BoneRole::RightKnee);
    std::cout << "[Locomotion] relevant shader bones (not direct animation targets)" << std::endl;
    for (const LocomotionBone& bone : locomotionBones) {
        if (bone.boneIndex < 0 || bone.boneIndex >= static_cast<int>(relevantBones.size())) continue;
        std::cout << "  - " << relevantBones[bone.boneIndex].name
            << " group=" << BoneGroupName(bone.group)
            << " side=" << BoneSideName(bone.side)
            << " animated=" << (bone.appliesOffset ? "yes" : "no")
            << " facialExcluded=" << (relevantBones[bone.boneIndex].excludedFromLocomotion ? "yes" : "no")
            << std::endl;
    }
    std::cout << "[Locomotion] primary animated node targets" << std::endl;
    for (const PrimaryLocomotionBone& bone : primaryLocomotionBones) {
        std::cout << "  - " << bone.name
            << " paletteIndex=" << bone.skinningIndex
            << " role=" << BoneRoleName(bone.role)
            << (bone.skinningIndex >= 0 ? "" : " (node-only hierarchy target)")
            << std::endl;
    }
    std::cout << "[LocomotionDebug] snapshot end" << std::endl;
    std::cout << "============================================================" << std::endl;
    std::cout.flush();
}

void Model::updateLocomotion(float deltaTime, bool hasMovementInput, float movementSpeedScale) {
    const bool isRunning = forceRunAnimation || movementSpeedScale >= locomotionSettings.runThreshold;
    const LocomotionState targetState = (!proceduralLocomotionEnabled || bindPoseMode || !hasMovementInput)
        ? LocomotionState::Idle
        : (isRunning ? LocomotionState::Run : LocomotionState::Walk);
    locomotionState = targetState;

    const bool moving = targetState != LocomotionState::Idle;
    walkBlend = Approach(walkBlend, moving ? 1.0f : 0.0f, deltaTime * (moving ? locomotionSettings.blendInSpeed : locomotionSettings.blendOutSpeed));
    runBlend = Approach(runBlend, isRunning && moving ? 1.0f : 0.0f, deltaTime * locomotionSettings.runBlendSpeed);
    locomotionBlend = walkBlend;

    if (moving) {
        const float frequency = locomotionSettings.walkFrequency + (locomotionSettings.runFrequency - locomotionSettings.walkFrequency) * runBlend;
        walkTime += deltaTime * frequency * std::max(0.25f, movementSpeedScale);
        locomotionPhase = walkTime;
        const float twoPi = 6.28318530718f;
        if (walkTime > twoPi) {
            walkTime = std::fmod(walkTime, twoPi);
            locomotionPhase = walkTime;
        }
    }
}

vmath::mat4 Model::proceduralOffsetForPrimaryBone(const PrimaryLocomotionBone& bone) const {
    if (!proceduralLocomotionEnabled || bindPoseMode) {
        return vmath::mat4::identity();
    }

    const bool running = locomotionState == LocomotionState::Run;
    const float blend = walkBlend;
    const float leftPhase = locomotionPhase;
    const float rightPhase = locomotionPhase + 3.14159265359f;
    const float armSwingDeg = locomotionSettings.walkArmSwingDeg + (locomotionSettings.runArmSwingDeg - locomotionSettings.walkArmSwingDeg) * runBlend;
    const float legSwingDeg = locomotionSettings.walkLegSwingDeg + (locomotionSettings.runLegSwingDeg - locomotionSettings.walkLegSwingDeg) * runBlend;
    const float kneeBendDeg = locomotionSettings.walkKneeBendDeg + (locomotionSettings.runKneeBendDeg - locomotionSettings.walkKneeBendDeg) * runBlend;
    const float elbowBaseDeg = locomotionSettings.walkElbowBaseBendDeg + (locomotionSettings.runElbowBaseBendDeg - locomotionSettings.walkElbowBaseBendDeg) * runBlend;
    const float elbowAddDeg = locomotionSettings.walkElbowSwingAddDeg + (locomotionSettings.runElbowSwingAddDeg - locomotionSettings.walkElbowSwingAddDeg) * runBlend;

    float pitch = 0.0f;
    float yaw = 0.0f;
    float roll = 0.0f;

    auto applyAxis = [&](float angle, int axis) {
        if (axis == 0) pitch += angle;
        else if (axis == 1) yaw += angle;
        else roll += angle;
    };

    switch (bone.role) {
    case BoneRole::Pelvis:
        if (locomotionDebugLevel == LocomotionDebugLevel::PelvisOnly || locomotionDebugLevel == LocomotionDebugLevel::LowerBodyOnly || locomotionDebugLevel == LocomotionDebugLevel::FullBody) {
            applyAxis(std::sin(locomotionPhase) * (locomotionSettings.walkPelvisYawDeg + (locomotionSettings.runPelvisYawDeg - locomotionSettings.walkPelvisYawDeg) * runBlend) * blend, locomotionAxes.pelvisYawAxis);
            applyAxis(std::cos(locomotionPhase * 2.0f) * locomotionSettings.walkPelvisRollDeg * blend, locomotionAxes.pelvisRollAxis);
        }
        break;
    case BoneRole::Spine:
    case BoneRole::Chest:
        if (runBlend > 0.0f) {
            pitch += locomotionSettings.runTorsoLeanDeg * runBlend * blend;
        }
        break;
    case BoneRole::LeftShoulder:
    case BoneRole::RightShoulder:
        break;
    case BoneRole::LeftUpperArm:
    case BoneRole::RightUpperArm: {
        const int downSign = bone.role == BoneRole::LeftUpperArm ? locomotionAxes.leftShoulderDownSign : locomotionAxes.rightShoulderDownSign;
        applyAxis(static_cast<float>(downSign * shoulderCorrectionSign) * locomotionSettings.shoulderDownDeg, locomotionAxes.shoulderDownAxis);
        if (locomotionDebugLevel == LocomotionDebugLevel::FullBody) {
            const float rightArmSwing = std::sin(locomotionPhase) * armSwingDeg;
            const float leftArmSwing = -rightArmSwing;
            const float swing = bone.role == BoneRole::LeftUpperArm ? leftArmSwing * locomotionAxes.leftArmSwingSign : rightArmSwing * locomotionAxes.rightArmSwingSign;
            applyAxis(swing * blend, locomotionAxes.armSwingAxis);
        }
        break;
    }
    case BoneRole::LeftElbow:
    case BoneRole::RightElbow:
        if (locomotionDebugLevel == LocomotionDebugLevel::FullBody) {
            const float phase = std::sin(locomotionPhase);
            const float bend = bone.role == BoneRole::RightElbow
                ? elbowBaseDeg + std::max(0.0f, -phase) * elbowAddDeg
                : elbowBaseDeg + std::max(0.0f, phase) * elbowAddDeg;
            const int sign = bone.role == BoneRole::LeftElbow ? locomotionAxes.leftElbowSign : locomotionAxes.rightElbowSign;
            applyAxis(static_cast<float>(sign) * std::min(bend, running ? 25.0f : 18.0f) * blend, locomotionAxes.elbowBendAxis);
        }
        break;
    case BoneRole::LeftHip:
    case BoneRole::RightHip: {
        if (locomotionDebugLevel == LocomotionDebugLevel::PelvisOnly) break;
        if (locomotionDebugLevel == LocomotionDebugLevel::LeftLegOnly && bone.role != BoneRole::LeftHip) break;
        const float leftLegSwing = std::sin(locomotionPhase) * legSwingDeg;
        const float rightLegSwing = -leftLegSwing;
        float swing = bone.role == BoneRole::LeftHip ? leftLegSwing * locomotionAxes.leftLegSign : rightLegSwing * locomotionAxes.rightLegSign;
        applyAxis(swing * blend, locomotionAxes.legSwingAxis);
        if (locomotionDebugLevel == LocomotionDebugLevel::LeftLegOnly) {
            pitch = 10.0f * blend;
        }
        break;
    }
    case BoneRole::LeftKnee:
    case BoneRole::RightKnee: {
        if (locomotionDebugLevel == LocomotionDebugLevel::PelvisOnly || locomotionDebugLevel == LocomotionDebugLevel::LeftLegOnly) break;
        const float kneePhase = bone.role == BoneRole::LeftKnee ? leftPhase : rightPhase;
        const int sign = bone.role == BoneRole::LeftKnee ? locomotionAxes.leftKneeSign : locomotionAxes.rightKneeSign;
        applyAxis(static_cast<float>(sign) * std::max(0.0f, std::sin(kneePhase + 0.78539816339f)) * kneeBendDeg * blend, locomotionAxes.kneeBendAxis);
        break;
    }
    default:
        break;
    }

    return vmath::rotate(pitch, 1.0f, 0.0f, 0.0f)
        * vmath::rotate(yaw, 0.0f, 1.0f, 0.0f)
        * vmath::rotate(roll, 0.0f, 0.0f, 1.0f);
}

void Model::computeFinalBoneMatrices() {
    if (finalBoneMatrices.size() != relevantBones.size()) {
        finalBoneMatrices.assign(relevantBones.size(), vmath::mat4::identity());
    }
    if (rootNode.name.empty()) return;
    computeFinalBoneMatricesRecursive(rootNode, vmath::mat4::identity());
}

void Model::computeFinalBoneMatricesRecursive(const SkeletonNode& node, const vmath::mat4& parentTransform) {
    vmath::mat4 offset = vmath::mat4::identity();
    if (!primaryLocomotionBones.empty()) {
        for (const PrimaryLocomotionBone& primaryBone : primaryLocomotionBones) {
            if (primaryBone.name == node.name) {
                if (singleBoneTestMode) {
                    if (primaryBone.role == singleBoneTestRole) {
                        if (singleBoneTestAxis == 0) {
                            offset = vmath::rotate(singleBoneTestAngleDeg, 1.0f, 0.0f, 0.0f);
                        }
                        else if (singleBoneTestAxis == 1) {
                            offset = vmath::rotate(singleBoneTestAngleDeg, 0.0f, 1.0f, 0.0f);
                        }
                        else {
                            offset = vmath::rotate(singleBoneTestAngleDeg, 0.0f, 0.0f, 1.0f);
                        }
                    }
                }
                else {
                    offset = proceduralOffsetForPrimaryBone(primaryBone);
                }
                break;
            }
        }
    }
    const vmath::mat4 animatedLocalTransform = node.baseTransform * offset;
    const auto foundBone = boneNameToIndex.find(node.name);

    const vmath::mat4 globalTransform = parentTransform * animatedLocalTransform;
    if (foundBone != boneNameToIndex.end()) {
        const int boneIndex = foundBone->second;
        if (boneIndex >= 0 && boneIndex < static_cast<int>(relevantBones.size())) {
            finalBoneMatrices[boneIndex] = globalInverseTransform * globalTransform * relevantBones[boneIndex].offsetMatrix;
        }
    }

    for (const SkeletonNode& child : node.children) {
        computeFinalBoneMatricesRecursive(child, globalTransform);
    }
}

void Model::uploadBoneMatrices(GLuint shaderProgram) const {
    const int boneCount = static_cast<int>(finalBoneMatrices.size());
    glUniform1i(glGetUniformLocation(shaderProgram, "useSkinning"), boneCount > 0 ? 1 : 0);
    glUniform1i(glGetUniformLocation(shaderProgram, "boneCount"), boneCount);
    glActiveTexture(GL_TEXTURE2);
    glBindBuffer(GL_TEXTURE_BUFFER, boneMatrixBuffer);
    if (boneCount > 0) {
        glBufferData(GL_TEXTURE_BUFFER, finalBoneMatrices.size() * sizeof(vmath::mat4), finalBoneMatrices.data(), GL_DYNAMIC_DRAW);
    }
    glBindTexture(GL_TEXTURE_BUFFER, boneMatrixTexture);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, boneMatrixBuffer);
    glUniform1i(glGetUniformLocation(shaderProgram, "boneMatricesTex"), 2);
    static int uploadPrintsRemaining = 4;
    if (uploadPrintsRemaining > 0) {
        std::cout << "[SkinningUpload] shader bone matrix count=" << boneCount << std::endl;
        --uploadPrintsRemaining;
    }
}

std::vector<Texture> Model::loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName) {
    std::vector<Texture> textures;
    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
        aiString str;
        mat->GetTexture(type, i, &str);
        std::string texturePath = NormalizeTexturePath(str.C_Str());

        bool skip = false;
        for (unsigned int j = 0; j < textures_loaded.size(); j++) {
            if (textures_loaded[j].path == texturePath && textures_loaded[j].type == typeName) {
                textures.push_back(textures_loaded[j]);
                skip = true;
                break;
            }
        }
        if (!skip) {
            Texture texture;
            texture.id = TextureFromFile(texturePath.c_str(), this->directory);
            texture.type = typeName;
            texture.path = texturePath;
            textures.push_back(texture);
            textures_loaded.push_back(texture);
        }
    }
    return textures;
}

void Model::setupFloor() {
    float floorVertices[] = {
        // position              // texCoord // normal
        -80.0f, -2.0f, -80.0f,   0.0f,  0.0f,  0.0f, 1.0f, 0.0f,
         80.0f, -2.0f, -80.0f,  24.0f,  0.0f,  0.0f, 1.0f, 0.0f,
         80.0f, -2.0f,  80.0f,  24.0f, 24.0f,  0.0f, 1.0f, 0.0f,
        -80.0f, -2.0f,  80.0f,   0.0f, 24.0f,  0.0f, 1.0f, 0.0f
    };
    unsigned int floorIndices[] = { 0, 1, 2, 0, 2, 3 };

    glGenVertexArrays(1, &floorVAO);
    glGenBuffers(1, &floorVBO);
    glGenBuffers(1, &floorEBO);

    glBindVertexArray(floorVAO);
    glBindBuffer(GL_ARRAY_BUFFER, floorVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(floorVertices), floorVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, floorEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(floorIndices), floorIndices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));

    glBindVertexArray(0);

    floorTexture = TextureFromFile("Floor.jpg", "../forBackground");
    floorReady = true;
}

void Model::drawFloor(GLuint shaderProgram) {
    if (!floorReady) return;

    vmath::mat4 floorModel = vmath::mat4::identity();
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, floorModel);
    glUniform3f(glGetUniformLocation(shaderProgram, "materialDiffuse"), 1.0f, 1.0f, 1.0f);
    glUniform1f(glGetUniformLocation(shaderProgram, "materialOpacity"), 1.0f);
    glUniform1i(glGetUniformLocation(shaderProgram, "useDiffuseTexture"), 1);
    glUniform1i(glGetUniformLocation(shaderProgram, "useAlphaTexture"), 0);
    glUniform1i(glGetUniformLocation(shaderProgram, "flipTextureV"), 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, floorTexture);
    glUniform1i(glGetUniformLocation(shaderProgram, "texture_diffuse1"), 0);

    glBindVertexArray(floorVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}
