#include "Model.h"
#include "BoneClassifier.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cmath>
#include <cstring>
#include <functional>
#include <iostream>
#include <windows.h>
#include <shader.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

using namespace BoneClassifier;

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#ifndef GL_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#endif
#ifndef GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#endif
#ifndef GL_MAX_SHADER_STORAGE_BLOCK_SIZE
#define GL_MAX_SHADER_STORAGE_BLOCK_SIZE 0x90DE
#endif
#ifndef GL_MAX_TEXTURE_BUFFER_SIZE
#define GL_MAX_TEXTURE_BUFFER_SIZE 0x8C2B
#endif

#pragma region Static Helpers
// �� cpp ���ο����� ���� ���� �Լ��� ����� ���� �����Դϴ�.

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

static const char* DebugJointTestModeName(DebugJointTestMode mode) {
    switch (mode) {
    case DebugJointTestMode::LeftKnee_X_Pos: return "LeftKnee_X_Pos";
    case DebugJointTestMode::LeftKnee_X_Neg: return "LeftKnee_X_Neg";
    case DebugJointTestMode::LeftKnee_Y_Pos: return "LeftKnee_Y_Pos";
    case DebugJointTestMode::LeftKnee_Y_Neg: return "LeftKnee_Y_Neg";
    case DebugJointTestMode::LeftKnee_Z_Pos: return "LeftKnee_Z_Pos";
    case DebugJointTestMode::LeftKnee_Z_Neg: return "LeftKnee_Z_Neg";
    case DebugJointTestMode::RightKnee_X_Pos: return "RightKnee_X_Pos";
    case DebugJointTestMode::RightKnee_X_Neg: return "RightKnee_X_Neg";
    case DebugJointTestMode::RightKnee_Y_Pos: return "RightKnee_Y_Pos";
    case DebugJointTestMode::RightKnee_Y_Neg: return "RightKnee_Y_Neg";
    case DebugJointTestMode::RightKnee_Z_Pos: return "RightKnee_Z_Pos";
    case DebugJointTestMode::RightKnee_Z_Neg: return "RightKnee_Z_Neg";
    case DebugJointTestMode::LeftElbow_X_Pos: return "LeftElbow_X_Pos";
    case DebugJointTestMode::LeftElbow_X_Neg: return "LeftElbow_X_Neg";
    case DebugJointTestMode::LeftElbow_Y_Pos: return "LeftElbow_Y_Pos";
    case DebugJointTestMode::LeftElbow_Y_Neg: return "LeftElbow_Y_Neg";
    case DebugJointTestMode::LeftElbow_Z_Pos: return "LeftElbow_Z_Pos";
    case DebugJointTestMode::LeftElbow_Z_Neg: return "LeftElbow_Z_Neg";
    case DebugJointTestMode::RightElbow_X_Pos: return "RightElbow_X_Pos";
    case DebugJointTestMode::RightElbow_X_Neg: return "RightElbow_X_Neg";
    case DebugJointTestMode::RightElbow_Y_Pos: return "RightElbow_Y_Pos";
    case DebugJointTestMode::RightElbow_Y_Neg: return "RightElbow_Y_Neg";
    case DebugJointTestMode::RightElbow_Z_Pos: return "RightElbow_Z_Pos";
    case DebugJointTestMode::RightElbow_Z_Neg: return "RightElbow_Z_Neg";
    default: return "None";
    }
}

static const char* LocalAxisName(LocalAxis axis) {
    switch (axis) {
    case LocalAxis::X: return "X";
    case LocalAxis::Y: return "Y";
    case LocalAxis::Z: return "Z";
    default: return "X";
    }
}

static LocalAxis LocalAxisFromIndex(int axis) {
    if (axis == 1) return LocalAxis::Y;
    if (axis == 2) return LocalAxis::Z;
    return LocalAxis::X;
}

static int LocalAxisToIndex(LocalAxis axis) {
    switch (axis) {
    case LocalAxis::Y: return 1;
    case LocalAxis::Z: return 2;
    default: return 0;
    }
}

static bool DebugJointModeToRoleAxisSign(DebugJointTestMode mode, BoneRole& role, LocalAxis& axis, float& sign) {
    switch (mode) {
    case DebugJointTestMode::LeftKnee_X_Pos: role = BoneRole::LeftKnee; axis = LocalAxis::X; sign = 1.0f; return true;
    case DebugJointTestMode::LeftKnee_X_Neg: role = BoneRole::LeftKnee; axis = LocalAxis::X; sign = -1.0f; return true;
    case DebugJointTestMode::LeftKnee_Y_Pos: role = BoneRole::LeftKnee; axis = LocalAxis::Y; sign = 1.0f; return true;
    case DebugJointTestMode::LeftKnee_Y_Neg: role = BoneRole::LeftKnee; axis = LocalAxis::Y; sign = -1.0f; return true;
    case DebugJointTestMode::LeftKnee_Z_Pos: role = BoneRole::LeftKnee; axis = LocalAxis::Z; sign = 1.0f; return true;
    case DebugJointTestMode::LeftKnee_Z_Neg: role = BoneRole::LeftKnee; axis = LocalAxis::Z; sign = -1.0f; return true;
    case DebugJointTestMode::RightKnee_X_Pos: role = BoneRole::RightKnee; axis = LocalAxis::X; sign = 1.0f; return true;
    case DebugJointTestMode::RightKnee_X_Neg: role = BoneRole::RightKnee; axis = LocalAxis::X; sign = -1.0f; return true;
    case DebugJointTestMode::RightKnee_Y_Pos: role = BoneRole::RightKnee; axis = LocalAxis::Y; sign = 1.0f; return true;
    case DebugJointTestMode::RightKnee_Y_Neg: role = BoneRole::RightKnee; axis = LocalAxis::Y; sign = -1.0f; return true;
    case DebugJointTestMode::RightKnee_Z_Pos: role = BoneRole::RightKnee; axis = LocalAxis::Z; sign = 1.0f; return true;
    case DebugJointTestMode::RightKnee_Z_Neg: role = BoneRole::RightKnee; axis = LocalAxis::Z; sign = -1.0f; return true;
    case DebugJointTestMode::LeftElbow_X_Pos: role = BoneRole::LeftElbow; axis = LocalAxis::X; sign = 1.0f; return true;
    case DebugJointTestMode::LeftElbow_X_Neg: role = BoneRole::LeftElbow; axis = LocalAxis::X; sign = -1.0f; return true;
    case DebugJointTestMode::LeftElbow_Y_Pos: role = BoneRole::LeftElbow; axis = LocalAxis::Y; sign = 1.0f; return true;
    case DebugJointTestMode::LeftElbow_Y_Neg: role = BoneRole::LeftElbow; axis = LocalAxis::Y; sign = -1.0f; return true;
    case DebugJointTestMode::LeftElbow_Z_Pos: role = BoneRole::LeftElbow; axis = LocalAxis::Z; sign = 1.0f; return true;
    case DebugJointTestMode::LeftElbow_Z_Neg: role = BoneRole::LeftElbow; axis = LocalAxis::Z; sign = -1.0f; return true;
    case DebugJointTestMode::RightElbow_X_Pos: role = BoneRole::RightElbow; axis = LocalAxis::X; sign = 1.0f; return true;
    case DebugJointTestMode::RightElbow_X_Neg: role = BoneRole::RightElbow; axis = LocalAxis::X; sign = -1.0f; return true;
    case DebugJointTestMode::RightElbow_Y_Pos: role = BoneRole::RightElbow; axis = LocalAxis::Y; sign = 1.0f; return true;
    case DebugJointTestMode::RightElbow_Y_Neg: role = BoneRole::RightElbow; axis = LocalAxis::Y; sign = -1.0f; return true;
    case DebugJointTestMode::RightElbow_Z_Pos: role = BoneRole::RightElbow; axis = LocalAxis::Z; sign = 1.0f; return true;
    case DebugJointTestMode::RightElbow_Z_Neg: role = BoneRole::RightElbow; axis = LocalAxis::Z; sign = -1.0f; return true;
    default: return false;
    }
}

static vmath::mat4 MakeLocalAxisRotation(LocalAxis axis, float signedAngleDeg) {
    switch (axis) {
    case LocalAxis::Y: return vmath::rotate(signedAngleDeg, 0.0f, 1.0f, 0.0f);
    case LocalAxis::Z: return vmath::rotate(signedAngleDeg, 0.0f, 0.0f, 1.0f);
    default: return vmath::rotate(signedAngleDeg, 1.0f, 0.0f, 0.0f);
    }
}

static vmath::mat4 MakeJointBendRotation(const JointBendConfig& cfg, float positiveBendAngleDeg) {
    const float clamped = std::max(cfg.minAngleDeg, std::min(cfg.maxAngleDeg, positiveBendAngleDeg));
    return MakeLocalAxisRotation(cfg.axis, clamped * cfg.sign);
}

static JointBendConfig MakeBendConfig(LocalAxis axis, float sign, float minAngleDeg = 0.0f, float maxAngleDeg = 30.0f) {
    JointBendConfig cfg;
    cfg.axis = axis;
    cfg.sign = sign;
    cfg.minAngleDeg = minAngleDeg;
    cfg.maxAngleDeg = maxAngleDeg;
    return cfg;
}

static vmath::vec3 TransformDirection(const vmath::mat4& matrix, const vmath::vec3& direction) {
    return vmath::vec3(
        matrix[0][0] * direction[0] + matrix[1][0] * direction[1] + matrix[2][0] * direction[2],
        matrix[0][1] * direction[0] + matrix[1][1] * direction[1] + matrix[2][1] * direction[2],
        matrix[0][2] * direction[0] + matrix[1][2] * direction[1] + matrix[2][2] * direction[2]
    );
}

static void PrintVec3(const char* label, const vmath::vec3& value) {
    std::cout << label << "=(" << value[0] << "," << value[1] << "," << value[2] << ")";
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

static float Clamp01(float value) {
    return std::max(0.0f, std::min(1.0f, value));
}

static float Approach(float current, float target, float delta) {
    if (current < target) {
        return std::min(current + delta, target);
    }
    return std::max(current - delta, target);
}

static bool AddBoneDataToVertex(Vertex& vertex, int boneID, float weight) {
    if (weight <= 0.0f || boneID < 0) return false;
    for (int i = 0; i < 4; ++i) {
        if (vertex.boneIDs[i] < 0) {
            vertex.boneIDs[i] = boneID;
            vertex.boneWeights[i] = weight;
            return true;
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
        return true;
    }
    return false;
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

#pragma endregion

#pragma region Model Lifecycle
// �� �ʱ�ȭ �� shader, Assimp �ε�, �� ��� ����, �ٴ� ���ҽ��� �غ��ϴ� �����Դϴ�.

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
    debugJointTestMode = DebugJointTestMode::None;
    skinningVertexCount = 0;
    zeroWeightVertexCount = 0;
    badWeightVertexCount = 0;
    droppedInfluenceCount = 0;
    boneIdsOverUploadedRange = 0;
    maxBoneIdUsedByVertices = -1;
    finalBoneMatrixUpdated.clear();
    updatedFinalBoneMatrixCount = 0;
    std::fill(std::begin(influenceHistogram), std::end(influenceHistogram), 0);
    minWeightSum = 9999.0f;
    maxWeightSum = 0.0f;
    totalWeightSum = 0.0;
    hasModelBounds = false;
    modelMinY = 0.0f;
    modelMaxY = 0.0f;

    loadModel(objFilePath);
    printGLLimits();
    if (boneMatrixBuffer == 0) {
        glGenBuffers(1, &boneMatrixBuffer);
    }
    if (boneMatrixTexture == 0) {
        glGenTextures(1, &boneMatrixTexture);
    }
    setupFloor();
}

#pragma endregion

#pragma region Rendering
// �����Ӻ� �� ���, ī�޶� ���, ����, �� ����� �����ϰ� mesh�� �׸��� �����Դϴ�.

void Model::draw(const ModelDrawParams& params) {
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glUseProgram(shaderProgram);
    updateLocomotion(params.deltaTime, params.hasMovementInput, params.movementSpeedScale);
    computeFinalBoneMatrices();

    const CharacterCameraFrame cameraFrame = Camera::buildCharacterFrame(
        params.camera,
        params.aspect,
        params.characterPosition,
        modelMinY,
        modelMaxY,
        hasModelBounds);
    vmath::mat4 model_matrix = vmath::translate(params.characterPosition[0], params.characterPosition[1] - 2.0f, params.characterPosition[2])
        * vmath::rotate(params.characterYawDegrees, 0.0f, 1.0f, 0.0f)
        * vmath::scale(1.0f);

    GLuint projLoc = glGetUniformLocation(shaderProgram, "projection");
    GLuint viewLoc = glGetUniformLocation(shaderProgram, "view");
    GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");

    glUniformMatrix4fv(projLoc, 1, GL_FALSE, cameraFrame.projection);
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, cameraFrame.view);
    glUniform3f(glGetUniformLocation(shaderProgram, "lightPos"), 18.0f, 28.0f, 12.0f);
    glUniform3f(glGetUniformLocation(shaderProgram, "viewPos"), cameraFrame.eye[0], cameraFrame.eye[1], cameraFrame.eye[2]);
    glUniform3f(glGetUniformLocation(shaderProgram, "lightColor"), 1.0f, 0.94f, 0.82f);
    glUniform1f(glGetUniformLocation(shaderProgram, "ambientStrength"), 0.28f);
    glUniform1f(glGetUniformLocation(shaderProgram, "specularStrength"), 0.35f);
    glUniform1f(glGetUniformLocation(shaderProgram, "shininess"), 32.0f);

    glUniform1i(glGetUniformLocation(shaderProgram, "useFirstPersonClip"), 0);
    glUniform1i(glGetUniformLocation(shaderProgram, "useSkinning"), 0);
    drawFloor(shaderProgram);
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, model_matrix);
    uploadBoneMatrices(shaderProgram);
    glUniform1i(glGetUniformLocation(shaderProgram, "useFirstPersonClip"), params.camera.firstPerson ? 1 : 0);
    glUniform1f(glGetUniformLocation(shaderProgram, "firstPersonClipMaxY"), cameraFrame.firstPersonChestClipY);

    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);

    if (cameraFrame.drawCharacterMesh) {
        drawSkeletonDebug(shaderProgram);
        for (unsigned int i = 0; i < meshes.size(); i++) {
            if (!meshes[i].isTransparent()) meshes[i].draw(shaderProgram);
        }
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    if (cameraFrame.drawCharacterMesh) {
        for (unsigned int i = 0; i < meshes.size(); i++) {
            if (meshes[i].isTransparent()) meshes[i].draw(shaderProgram);
        }
    }
    glDepthMask(GL_TRUE);
}

#pragma endregion

#pragma region Model Loading
// �ܺ� �� ���ϰ� skeleton node�� �о� ���� ������ ������ ��ȯ�ϴ� �����Դϴ�.

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
    printBoneLimitSummary();
    printFullSkeletonTable(scene);
    buildPrimaryLocomotionRig();
    printJointBasisDiagnostics();
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

#pragma endregion

#pragma region For Debug
// ��Ÿ�� ���� Ȯ�ΰ� PMX/��/�α� ������ ���� ����� ���� �Լ� �����Դϴ�.

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

    if (role == BoneRole::LeftElbow || role == BoneRole::RightElbow || role == BoneRole::LeftKnee || role == BoneRole::RightKnee) {
        const bool uploaded = primary->skinningIndex >= 0 && primary->skinningIndex < static_cast<int>(finalBoneMatrices.size());
        const bool hasOffset = meshBone && primary->skinningIndex < static_cast<int>(relevantBones.size());
        std::cout << "[BoneTarget] " << BoneRoleName(role)
            << " selected=" << primary->name
            << " index=" << primary->skinningIndex
            << " weighted=" << (weightCount > 0 ? "true" : "false")
            << " uploaded=" << (uploaded ? "true" : "false")
            << std::endl;
        std::cout << "[BoneTargetCheck] " << BoneRoleName(role)
            << " finalMatrixIndex=" << primary->skinningIndex
            << " inRange=" << (uploaded ? "true" : "false")
            << " hasNode=" << (node ? "true" : "false")
            << " hasOffset=" << (hasOffset ? "true" : "false")
            << " weightCount=" << weightCount
            << std::endl;
    }
}

void Model::printFocusedJointDebug(BoneRole role) const {
    std::cout << "\n============================================================" << std::endl;
    std::cout << "[FocusedJointDebug] role=" << BoneRoleName(role) << std::endl;
    std::vector<const PrimaryLocomotionBone*> targets;
    for (const PrimaryLocomotionBone& bone : primaryLocomotionBones) {
        if (bone.role == role) {
            targets.push_back(&bone);
        }
    }
    if (targets.empty()) {
        std::cout << "[FocusedJointDebug] missing primary target" << std::endl;
        std::cout << "============================================================" << std::endl;
        return;
    }

    for (const PrimaryLocomotionBone* primary : targets) {
        const SkeletonNode* parent = nullptr;
        const SkeletonNode* node = findSkeletonNodeByName(rootNode, primary->name, &parent);
        const bool inRange = primary->skinningIndex >= 0 && primary->skinningIndex < static_cast<int>(relevantBones.size());
        const unsigned int weightCount = inRange ? relevantBones[primary->skinningIndex].weightCount : 0;
        std::cout << "[FocusedJointDebug] target=" << primary->name
            << " paletteIndex=" << primary->skinningIndex
            << " weighted=" << (weightCount > 0 ? "true" : "false")
            << " weights=" << weightCount
            << " meshIndex=" << (inRange ? relevantBones[primary->skinningIndex].meshIndex : -1)
            << " sourceBoneIndex=" << (inRange ? relevantBones[primary->skinningIndex].sourceIndex : -1)
            << " uploaded=" << (primary->skinningIndex >= 0 && primary->skinningIndex < static_cast<int>(finalBoneMatrices.size()) ? "true" : "false")
            << " hasNode=" << (node ? "true" : "false")
            << " parent=" << (parent ? parent->name : "<none>")
            << " bendStage=" << primary->bendStage
            << " bendWeight=" << primary->bendWeight
            << std::endl;

        if (!node) {
            continue;
        }

        for (const SkeletonNode& child : node->children) {
            const auto foundChild = boneNameToIndex.find(child.name);
            if (foundChild != boneNameToIndex.end()) {
                const int childIndex = foundChild->second;
                const bool childInRange = childIndex >= 0 && childIndex < static_cast<int>(relevantBones.size());
                std::cout << "[FocusedJointDebug] child=" << child.name
                    << " paletteIndex=" << childIndex
                    << " weights=" << (childInRange ? relevantBones[childIndex].weightCount : 0)
                    << " meshIndex=" << (childInRange ? relevantBones[childIndex].meshIndex : -1)
                    << std::endl;
            }
            else {
                std::cout << "[FocusedJointDebug] child=" << child.name
                    << " paletteIndex=<none>"
                    << std::endl;
            }
        }

        int descendantCount = 0;
        std::function<void(const SkeletonNode&)> printDescendants = [&](const SkeletonNode& current) {
            if (descendantCount >= 16) return;
            const auto found = boneNameToIndex.find(current.name);
            if (found != boneNameToIndex.end()) {
                const int index = found->second;
                const bool descendantInRange = index >= 0 && index < static_cast<int>(relevantBones.size());
                if (descendantInRange && relevantBones[index].weightCount > 0) {
                    std::cout << "[FocusedJointDebug] affectedBone=" << current.name
                        << " paletteIndex=" << index
                        << " weights=" << relevantBones[index].weightCount
                        << " meshIndex=" << relevantBones[index].meshIndex
                        << std::endl;
                    ++descendantCount;
                }
            }
            for (const SkeletonNode& child : current.children) {
                printDescendants(child);
            }
        };
        printDescendants(*node);

        if (primary->hasBendOverride) {
            std::cout << "[FocusedJointDebug] bendOverride axis=" << LocalAxisName(primary->bendOverride.axis)
                << " sign=" << primary->bendOverride.sign
                << " minDeg=" << primary->bendOverride.minAngleDeg
                << " maxDeg=" << primary->bendOverride.maxAngleDeg
                << std::endl;
        }
    }

    JointBendConfig cfg;
    bool hasBendConfig = true;
    switch (role) {
    case BoneRole::LeftElbow: cfg = bendConfig.leftElbow; break;
    case BoneRole::RightElbow: cfg = bendConfig.rightElbow; break;
    case BoneRole::LeftKnee: cfg = bendConfig.leftKnee; break;
    case BoneRole::RightKnee: cfg = bendConfig.rightKnee; break;
    default: hasBendConfig = false; break;
    }
    if (hasBendConfig) {
        std::cout << "[FocusedJointDebug] bendConfig axis=" << LocalAxisName(cfg.axis)
            << " sign=" << cfg.sign
            << " minDeg=" << cfg.minAngleDeg
            << " maxDeg=" << cfg.maxAngleDeg
            << std::endl;
    }
    std::cout << "============================================================" << std::endl;
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
    std::cout << "[VertexWeights] totalVertices=" << skinningVertexCount
        << ", verticesWithNoBone=" << zeroWeightVertexCount
        << ", maxBoneIdUsed=" << maxBoneIdUsedByVertices
        << ", boneIdsOverUploadedRange=" << boneIdsOverUploadedRange
        << ", droppedInfluences=" << droppedInfluenceCount
        << ", maxInfluencesPerVertex=4"
        << std::endl;
}

void Model::printBoneLimitSummary() const {
    std::cout << "============================================================" << std::endl;
    std::cout << "[BoneLimitSearch] MAX_BONES / MAX_BONE / BONE_LIMIT fixed cap found in project scan: no" << std::endl;
    std::cout << "[BoneLimitSearch] GLSL uniform mat4 bones[100] / finalBonesMatrices[100] found: no" << std::endl;
    std::cout << "[BoneLimitSearch] upload method = GL_TEXTURE_BUFFER + samplerBuffer boneMatricesTex" << std::endl;
    std::cout << "[BoneLimitSearch] finalBoneMatrices is dynamic, current size=" << finalBoneMatrices.size() << std::endl;
    std::cout << "[BoneLimitSearch] shader bone IDs reference finalBoneMatrices by palette index" << std::endl;
}

void Model::printGLLimits() const {
    GLint maxVertexUniformComponents = 0;
    GLint maxUniformBlockSize = 0;
    GLint maxShaderStorageBlockSize = 0;
    GLint maxTextureBufferSize = 0;
    glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &maxVertexUniformComponents);
    glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &maxUniformBlockSize);
    glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &maxShaderStorageBlockSize);
    glGetIntegerv(GL_MAX_TEXTURE_BUFFER_SIZE, &maxTextureBufferSize);

    std::cout << "============================================================" << std::endl;
    std::cout << "[GL Limits] GL_MAX_VERTEX_UNIFORM_COMPONENTS = " << maxVertexUniformComponents << std::endl;
    std::cout << "[GL Limits] Approx mat4 bones via vertex uniforms = " << (maxVertexUniformComponents / 16) << std::endl;
    std::cout << "[GL Limits] GL_MAX_UNIFORM_BLOCK_SIZE = " << maxUniformBlockSize << " bytes" << std::endl;
    std::cout << "[GL Limits] Approx mat4 bones via UBO = " << (maxUniformBlockSize / 64) << std::endl;
    std::cout << "[GL Limits] GL_MAX_SHADER_STORAGE_BLOCK_SIZE = " << maxShaderStorageBlockSize << " bytes" << std::endl;
    std::cout << "[GL Limits] GL_MAX_TEXTURE_BUFFER_SIZE = " << maxTextureBufferSize << " vec4 texels" << std::endl;
    std::cout << "[GL Limits] Approx mat4 bones via texture buffer = " << (maxTextureBufferSize / 4) << std::endl;
}

void Model::printBoneMatrixUpdateStats() const {
    unsigned int staleWeightedBones = 0;
    for (const BoneData& bone : relevantBones) {
        const bool updated = bone.animationIndex >= 0
            && bone.animationIndex < static_cast<int>(finalBoneMatrixUpdated.size())
            && finalBoneMatrixUpdated[bone.animationIndex] != 0;
        if (!updated && bone.weightCount > 0) {
            ++staleWeightedBones;
        }
    }
    std::cout << "[BoneStats] finalBoneMatrices.size = " << finalBoneMatrices.size() << std::endl;
    std::cout << "[BoneStats] uploadMethod = TextureBuffer" << std::endl;
    std::cout << "[BoneStats] uploadedBoneMatrices = " << finalBoneMatrices.size() << std::endl;
    std::cout << "[BoneStats] updatedByNodeTraversal = " << updatedFinalBoneMatrixCount << std::endl;
    std::cout << "[BoneStats] weightedBonesNotUpdatedByNodeTraversal = " << staleWeightedBones << std::endl;
    std::cout << "[BoneStats] maxBoneIdUsedByVertices = " << maxBoneIdUsedByVertices << std::endl;
    std::cout << "[BoneStats] boneIdsOverUploadedRange = " << boneIdsOverUploadedRange << std::endl;
}

void Model::printCriticalBoneDiagnostics() const {
    struct CriticalBoneName {
        const char* label;
        std::string name;
    };

    const std::string left = "\xE5\xB7\xA6";
    const std::string right = "\xE5\x8F\xB3";
    const std::string arm = "\xE8\x85\x95";
    const std::string elbowHiragana = "\xE3\x81\xB2\xE3\x81\x98";
    const std::string wrist = "\xE6\x89\x8B\xE9\xA6\x96";
    const std::string foot = "\xE8\xB6\xB3";
    const std::string kneeHiragana = "\xE3\x81\xB2\xE3\x81\x96";
    const std::string ankle = "\xE8\xB6\xB3\xE9\xA6\x96";
    const std::string waist = "\xE8\x85\xB0";
    const std::string lowerBody = "\xE4\xB8\x8B\xE5\x8D\x8A\xE8\xBA\xAB";
    const std::string center = "\xE3\x82\xBB\xE3\x83\xB3\xE3\x82\xBF\xE3\x83\xBC";
    const std::string cancel = "\xE3\x82\xAD\xE3\x83\xA3\xE3\x83\xB3\xE3\x82\xBB\xE3\x83\xAB";
    const std::string parent = "\xE8\xA6\xAA";
    const std::string fullwidthIK = "\xEF\xBC\xA9\xEF\xBC\xAB";
    const std::string toe = "\xE3\x81\xA4\xE3\x81\xBE\xE5\x85\x88";

    const std::vector<CriticalBoneName> criticalBones = {
        { "LeftArm", left + arm },
        { "RightArm", right + arm },
        { "LeftElbow", left + elbowHiragana },
        { "RightElbow", right + elbowHiragana },
        { "LeftWrist", left + wrist },
        { "RightWrist", right + wrist },
        { "LeftArmD", left + arm + "D" },
        { "RightArmD", right + arm + "D" },
        { "LeftElbowD", left + elbowHiragana + "D" },
        { "RightElbowD", right + elbowHiragana + "D" },
        { "LeftWristD", left + wrist + "D" },
        { "RightWristD", right + wrist + "D" },
        { "LeftLeg", left + foot },
        { "RightLeg", right + foot },
        { "LeftKnee", left + kneeHiragana },
        { "RightKnee", right + kneeHiragana },
        { "LeftAnkle", left + ankle },
        { "RightAnkle", right + ankle },
        { "LeftLegD", left + foot + "D" },
        { "RightLegD", right + foot + "D" },
        { "LeftKneeD", left + kneeHiragana + "D" },
        { "RightKneeD", right + kneeHiragana + "D" },
        { "LeftAnkleD", left + ankle + "D" },
        { "RightAnkleD", right + ankle + "D" },
        { "Waist", waist },
        { "LowerBody", lowerBody },
        { "Center", center },
        { "LeftWaistCancel", waist + cancel + left },
        { "RightWaistCancel", waist + cancel + right },
        { "LeftFootIKParent", left + foot + "IK" + parent },
        { "RightFootIKParent", right + foot + "IK" + parent },
        { "LeftFootFullwidthIK", left + foot + fullwidthIK },
        { "RightFootFullwidthIK", right + foot + fullwidthIK },
        { "LeftToeFullwidthIK", left + toe + fullwidthIK },
        { "RightToeFullwidthIK", right + toe + fullwidthIK },
    };

    std::cout << "============================================================" << std::endl;
    std::cout << "[CriticalBoneDiagnostics] begin" << std::endl;
    for (const CriticalBoneName& critical : criticalBones) {
        const auto found = boneNameToIndex.find(critical.name);
        if (found == boneNameToIndex.end()) {
            std::cout << "[CriticalBone] label=" << critical.label
                << " name=" << critical.name
                << " foundInMeshBones=false"
                << std::endl;
            continue;
        }

        const int index = found->second;
        const bool inRange = index >= 0 && index < static_cast<int>(relevantBones.size());
        const unsigned int weights = inRange ? relevantBones[index].weightCount : 0;
        const bool uploaded = index >= 0 && index < static_cast<int>(finalBoneMatrices.size());
        const bool updated = index >= 0
            && index < static_cast<int>(finalBoneMatrixUpdated.size())
            && finalBoneMatrixUpdated[index] != 0;
        std::cout << "[CriticalBone] label=" << critical.label
            << " name=" << critical.name
            << " foundInMeshBones=true"
            << " weights=" << weights
            << " gpuIndex=" << index
            << " withinUploadedRange=" << (uploaded ? "true" : "false")
            << " updatedByNodeTraversal=" << (updated ? "true" : "false")
            << " excludedKeyword=" << (inRange && relevantBones[index].excludedFromLocomotion ? "true" : "false")
            << std::endl;
    }
    std::cout << "[CriticalBoneDiagnostics] end" << std::endl;
}

#pragma endregion

#pragma region Skeleton Debug Rendering
// skeleton ������ ���� vertex�� ����� ����� �������� �������ϴ� �����Դϴ�.

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

#pragma endregion

#pragma region Locomotion Rig Setup
// �� �� �̸��� ���� ���ҿ� �����ϰ� ���� ���� ��/��ȣ�� �����ϴ� �����Դϴ�.

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
        const SkeletonNode* parent = nullptr;
        const SkeletonNode* node = findSkeletonNodeByName(rootNode, name, &parent);
        const bool hasNode = node != nullptr;
        const bool isBendRole = role == BoneRole::LeftElbow
            || role == BoneRole::RightElbow
            || role == BoneRole::LeftKnee
            || role == BoneRole::RightKnee;
        const bool dSuffix = !name.empty() && name.back() == 'D';

        if (!hasNode) {
            score += 3000;
        }
        if (isBendRole && dSuffix) {
            score -= 700;
        }
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
        if (role == BoneRole::LeftElbow && (name == "\xE5\xB7\xA6\xE3\x81\xB2\xE3\x81\x98" || name == "\xE5\xB7\xA6\xE8\x82\x98")) score -= 300;
        if (role == BoneRole::RightElbow && (name == "\xE5\x8F\xB3\xE3\x81\xB2\xE3\x81\x98" || name == "\xE5\x8F\xB3\xE8\x82\x98")) score -= 300;
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

    auto addPrimaryByName = [&](BoneRole role, const std::string& name, float bendWeight, int bendStage, const JointBendConfig* bendOverride) {
        if (name.empty()) return false;
        const bool hasPaletteBone = boneNameToIndex.find(name) != boneNameToIndex.end();
        const bool hasSkeletonNode = findSkeletonNodeByName(rootNode, name, nullptr) != nullptr;
        if (!hasPaletteBone && !hasSkeletonNode) return false;
        const auto duplicate = std::find_if(primaryLocomotionBones.begin(), primaryLocomotionBones.end(), [&](const PrimaryLocomotionBone& existing) {
            return existing.name == name && existing.role == role;
        });
        if (duplicate != primaryLocomotionBones.end()) return false;
        const auto foundSkinning = boneNameToIndex.find(name);
        PrimaryLocomotionBone bone;
        bone.name = name;
        bone.skinningIndex = foundSkinning != boneNameToIndex.end() ? foundSkinning->second : -1;
        bone.role = role;
        bone.bendWeight = bendWeight;
        bone.bendStage = bendStage;
        if (bendOverride) {
            bone.bendOverride = *bendOverride;
            bone.hasBendOverride = true;
        }
        primaryLocomotionBones.push_back(bone);
        return true;
    };

    auto addPrimary = [&](BoneRole role) {
        const auto foundName = selectedNames.find(role);
        if (foundName == selectedNames.end()) return;
        addPrimaryByName(role, foundName->second.name, 1.0f, 0, nullptr);
    };

    auto addElbowChain = [&](BoneRole role, bool leftSide) {
        const std::string side = leftSide ? "\xE5\xB7\xA6" : "\xE5\x8F\xB3";
        const std::string elbowHiragana = "\xE3\x81\xB2\xE3\x81\x98";
        const std::string elbowKanji = "\xE8\x82\x98";
        const std::string adiName = leftSide ? "Bone_ADIelbow_L" : "Bone_ADIelbow_R";
        const std::string adiForearmName = leftSide ? "Bone_ADIarm002_L" : "Bone_ADIarm002_R";
        const std::string adiForearmEndName = leftSide ? "Bone_ADIarm003_L" : "Bone_ADIarm003_R";

        const JointBendConfig stage1Cfg = MakeBendConfig(LocalAxis::X, -1.0f, 0.0f, 60.0f);
        const JointBendConfig stage2Cfg = MakeBendConfig(LocalAxis::Y, leftSide ? -1.0f : 1.0f, 0.0f, 60.0f);

        bool added = false;
        added = addPrimaryByName(role, side + elbowHiragana, 1.0f, 1, &stage1Cfg) || added;
        added = addPrimaryByName(role, side + elbowKanji, 1.0f, 1, &stage1Cfg) || added;
        added = addPrimaryByName(role, adiForearmName, 0.65f, 2, &stage2Cfg) || added;
        added = addPrimaryByName(role, adiForearmEndName, 0.35f, 3, &stage2Cfg) || added;
        added = addPrimaryByName(role, adiName, 0.25f, 4, &stage2Cfg) || added;

        if (!added) {
            const auto foundName = selectedNames.find(role);
            if (foundName != selectedNames.end()) {
                const JointBendConfig fallbackCfg = leftSide ? bendConfig.leftElbow : bendConfig.rightElbow;
                addPrimaryByName(role, foundName->second.name, 1.0f, 1, &fallbackCfg);
            }
        }
    };

    addPrimary(BoneRole::Pelvis);
    addPrimary(BoneRole::Root);
    addPrimary(BoneRole::Spine);
    addPrimary(BoneRole::Chest);
    addPrimary(BoneRole::LeftShoulder);
    addPrimary(BoneRole::RightShoulder);
    addPrimary(BoneRole::LeftUpperArm);
    addPrimary(BoneRole::RightUpperArm);
    addElbowChain(BoneRole::LeftElbow, true);
    addElbowChain(BoneRole::RightElbow, false);
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
            << " bendStage=" << bone.bendStage
            << " bendWeight=" << bone.bendWeight
            << (bone.hasBendOverride ? " bendOverride=yes" : "")
            << (bone.skinningIndex >= 0 ? "" : " (node-only hierarchy target)")
            << std::endl;
    }

    autoConfigureJointBendsFromSkeleton();
}

void Model::autoConfigureJointBendsFromSkeleton() {
    auto translationOf = [](const vmath::mat4& matrix) {
        return vmath::vec3(matrix[3][0], matrix[3][1], matrix[3][2]);
    };

    struct PoseResult {
        const SkeletonNode* node = nullptr;
        vmath::mat4 parentGlobal = vmath::mat4::identity();
        vmath::mat4 nodeGlobal = vmath::mat4::identity();
    };

    std::function<bool(const SkeletonNode&, const std::string&, const vmath::mat4&, PoseResult&)> findPose =
        [&](const SkeletonNode& node, const std::string& targetName, const vmath::mat4& parentGlobal, PoseResult& result) {
            const vmath::mat4 nodeGlobal = parentGlobal * node.baseTransform;
            if (node.name == targetName) {
                result.node = &node;
                result.parentGlobal = parentGlobal;
                result.nodeGlobal = nodeGlobal;
                return true;
            }
            for (const SkeletonNode& child : node.children) {
                if (findPose(child, targetName, nodeGlobal, result)) {
                    return true;
                }
            }
            return false;
        };

    auto roleConfig = [&](BoneRole role) -> JointBendConfig* {
        switch (role) {
        case BoneRole::LeftKnee: return &bendConfig.leftKnee;
        case BoneRole::RightKnee: return &bendConfig.rightKnee;
        case BoneRole::LeftElbow: return &bendConfig.leftElbow;
        case BoneRole::RightElbow: return &bendConfig.rightElbow;
        default: return nullptr;
        }
    };

    auto estimateForRole = [&](BoneRole role, bool applySign) {
        const PrimaryLocomotionBone* primary = findPrimaryBoneByRole(role);
        JointBendConfig* cfg = roleConfig(role);
        if (!primary || !cfg || rootNode.name.empty()) {
            std::cout << "[AutoBendConfig] role=" << BoneRoleName(role) << " skipped: missing target" << std::endl;
            return;
        }

        PoseResult pose;
        if (!findPose(rootNode, primary->name, vmath::mat4::identity(), pose) || !pose.node || pose.node->children.empty()) {
            std::cout << "[AutoBendConfig] role=" << BoneRoleName(role)
                << " target=" << primary->name
                << " skipped: no node child to score"
                << std::endl;
            return;
        }

        const SkeletonNode& child = pose.node->children.front();
        const vmath::mat4 baseChildGlobal = pose.nodeGlobal * child.baseTransform;
        const vmath::vec3 basePos = translationOf(baseChildGlobal);

        float bestScore = -999999.0f;
        LocalAxis bestAxis = cfg->axis;
        float bestSign = cfg->sign;
        for (int axisIndex = 0; axisIndex < 3; ++axisIndex) {
            const LocalAxis axis = LocalAxisFromIndex(axisIndex);
            for (float sign : { 1.0f, -1.0f }) {
                const vmath::mat4 rotatedNodeGlobal = pose.parentGlobal * pose.node->baseTransform * MakeLocalAxisRotation(axis, 45.0f * sign);
                const vmath::mat4 testChildGlobal = rotatedNodeGlobal * child.baseTransform;
                const vmath::vec3 delta = translationOf(testChildGlobal) - basePos;
                const float sagittalMotion = std::fabs(delta[2]) + 0.65f * std::fabs(delta[1]);
                const float lateralPenalty = 0.55f * std::fabs(delta[0]);
                const float score = sagittalMotion - lateralPenalty;
                std::cout << "[AutoBendScore] role=" << BoneRoleName(role)
                    << " target=" << primary->name
                    << " child=" << child.name
                    << " axis=" << LocalAxisName(axis)
                    << " sign=" << (sign > 0.0f ? "+" : "-")
                    << " deltaX=" << delta[0]
                    << " deltaY=" << delta[1]
                    << " deltaZ=" << delta[2]
                    << " score=" << score
                    << std::endl;
                if (score > bestScore) {
                    bestScore = score;
                    bestAxis = axis;
                    bestSign = sign;
                }
            }
        }

        cfg->axis = bestAxis;
        if (applySign) {
            cfg->sign = bestSign;
        }
        std::cout << "[AutoBendConfig] role=" << BoneRoleName(role)
            << " target=" << primary->name
            << " axis=" << LocalAxisName(cfg->axis)
            << " sign=" << cfg->sign
            << " score=" << bestScore
            << " applied=true"
            << std::endl;
    };

    estimateForRole(BoneRole::LeftKnee, true);
    bendConfig.leftKnee = MakeBendConfig(LocalAxis::X, 1.0f, 0.0f, 30.0f);
    bendConfig.rightKnee = MakeBendConfig(LocalAxis::X, 1.0f, 0.0f, 30.0f);
    std::cout << "[ManualBendConfig] role=LeftKnee axis=X sign=+ applied from visual debug" << std::endl;
    std::cout << "[ManualBendConfig] role=RightKnee matchedFrom=LeftKnee axis="
        << LocalAxisName(bendConfig.rightKnee.axis)
        << " sign=" << bendConfig.rightKnee.sign
        << " applied=true" << std::endl;

    estimateForRole(BoneRole::LeftElbow, true);
    estimateForRole(BoneRole::RightElbow, true);
    bendConfig.leftElbow = MakeBendConfig(LocalAxis::X, -1.0f, 0.0f, 60.0f);
    bendConfig.rightElbow = MakeBendConfig(LocalAxis::X, -1.0f, 0.0f, 60.0f);
    std::cout << "[ManualBendConfig] role=LeftElbow stage1 axis=X sign=- applied from visual debug" << std::endl;
    std::cout << "[ManualBendConfig] role=RightElbow stage1 axis=X sign=- applied from visual debug" << std::endl;

    locomotionAxes.kneeBendAxis = LocalAxisToIndex(bendConfig.leftKnee.axis);
    locomotionAxes.elbowBendAxis = LocalAxisToIndex(bendConfig.leftElbow.axis);
}

#pragma endregion

#pragma region For Debug
// ��Ÿ�� ���� Ȯ�ΰ� PMX/��/�α� ������ ���� ����� ���� �Լ� �����Դϴ�.

void Model::printJointBasisDiagnostics() const {
    auto translationOf = [](const vmath::mat4& matrix) {
        return vmath::vec3(matrix[3][0], matrix[3][1], matrix[3][2]);
    };

    struct PoseResult {
        const SkeletonNode* node = nullptr;
        const SkeletonNode* parentNode = nullptr;
        vmath::mat4 parentGlobal = vmath::mat4::identity();
        vmath::mat4 nodeGlobal = vmath::mat4::identity();
    };

    std::function<bool(const SkeletonNode&, const SkeletonNode*, const std::string&, const vmath::mat4&, PoseResult&)> findPose =
        [&](const SkeletonNode& node, const SkeletonNode* parentNode, const std::string& targetName, const vmath::mat4& parentGlobal, PoseResult& result) {
            const vmath::mat4 nodeGlobal = parentGlobal * node.baseTransform;
            if (node.name == targetName) {
                result.node = &node;
                result.parentNode = parentNode;
                result.parentGlobal = parentGlobal;
                result.nodeGlobal = nodeGlobal;
                return true;
            }
            for (const SkeletonNode& child : node.children) {
                if (findPose(child, &node, targetName, nodeGlobal, result)) {
                    return true;
                }
            }
            return false;
        };

    auto printRoleBasis = [&](BoneRole role) {
        const PrimaryLocomotionBone* primary = findPrimaryBoneByRole(role);
        if (!primary || rootNode.name.empty()) {
            std::cout << "[JointBasis] " << BoneRoleName(role) << " missing target" << std::endl;
            return;
        }

        PoseResult pose;
        if (!findPose(rootNode, nullptr, primary->name, vmath::mat4::identity(), pose) || !pose.node) {
            std::cout << "[JointBasis] " << BoneRoleName(role)
                << " target=" << primary->name
                << " hasNode=false"
                << std::endl;
            return;
        }

        const SkeletonNode* childNode = pose.node->children.empty() ? nullptr : &pose.node->children.front();
        const vmath::mat4 childGlobal = childNode ? pose.nodeGlobal * childNode->baseTransform : pose.nodeGlobal;
        const vmath::vec3 parentPos = translationOf(pose.parentGlobal);
        const vmath::vec3 jointPos = translationOf(pose.nodeGlobal);
        const vmath::vec3 childPos = translationOf(childGlobal);
        const vmath::vec3 localX = TransformDirection(pose.nodeGlobal, vmath::vec3(1.0f, 0.0f, 0.0f));
        const vmath::vec3 localY = TransformDirection(pose.nodeGlobal, vmath::vec3(0.0f, 1.0f, 0.0f));
        const vmath::vec3 localZ = TransformDirection(pose.nodeGlobal, vmath::vec3(0.0f, 0.0f, 1.0f));

        std::cout << "[JointBasis] " << BoneRoleName(role)
            << " target=" << primary->name
            << " parent=" << (pose.parentNode ? pose.parentNode->name : "<none>")
            << " child=" << (childNode ? childNode->name : "<none>")
            << std::endl;
        std::cout << "  ";
        PrintVec3("parentGlobalPos", parentPos);
        std::cout << " ";
        PrintVec3("jointGlobalPos", jointPos);
        std::cout << " ";
        PrintVec3("childGlobalPos", childPos);
        std::cout << std::endl;
        std::cout << "  ";
        PrintVec3("localX_world", localX);
        std::cout << " ";
        PrintVec3("localY_world", localY);
        std::cout << " ";
        PrintVec3("localZ_world", localZ);
        std::cout << std::endl;
    };

    std::cout << "============================================================" << std::endl;
    std::cout << "[JointBasis] diagnostics begin" << std::endl;
    printRoleBasis(BoneRole::LeftKnee);
    printRoleBasis(BoneRole::RightKnee);
    printRoleBasis(BoneRole::LeftElbow);
    printRoleBasis(BoneRole::RightElbow);
    std::cout << "[JointBasis] diagnostics end" << std::endl;
}

#pragma endregion

#pragma region Assimp Processing
// Assimp scene/node/mesh/material/bone �����͸� ������Ʈ�� Mesh�� BoneData�� ��ȯ�ϴ� �����Դϴ�.

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
                if (!AddBoneDataToVertex(vertices[weight.mVertexId], animationBoneID, weight.mWeight)) {
                    ++droppedInfluenceCount;
                }
            }
        }
    }

    for (Vertex& vertex : vertices) {
        const float originalWeightSum = NormalizeVertexBoneWeights(vertex);
        int influenceCount = 0;
        for (int i = 0; i < 4; ++i) {
            if (vertex.boneIDs[i] >= 0 && vertex.boneWeights[i] > 0.0f) {
                ++influenceCount;
                maxBoneIdUsedByVertices = std::max(maxBoneIdUsedByVertices, vertex.boneIDs[i]);
                if (vertex.boneIDs[i] >= static_cast<int>(finalBoneMatrices.size())) {
                    ++boneIdsOverUploadedRange;
                }
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

    std::cout << "============================================================" << std::endl;
    std::cout << "[Assimp] scene meshes = " << scene->mNumMeshes << std::endl;
    for (unsigned int meshIndex = 0; meshIndex < scene->mNumMeshes; meshIndex++) {
        aiMesh* mesh = scene->mMeshes[meshIndex];
        std::cout << "[Assimp] mesh=" << meshIndex
            << " vertices=" << mesh->mNumVertices
            << " bones=" << mesh->mNumBones
            << std::endl;
        for (unsigned int boneIndex = 0; boneIndex < mesh->mNumBones; boneIndex++) {
            aiBone* bone = mesh->mBones[boneIndex];
            const std::string boneName = bone->mName.C_Str();

            std::cout << "[MeshBone] mesh=" << meshIndex
                << " localBone=" << boneIndex
                << " name=" << boneName
                << " weights=" << bone->mNumWeights
                << std::endl;

            const auto alreadyLoaded = boneNameToIndex.find(boneName);
            if (alreadyLoaded != boneNameToIndex.end()) {
                const int existingIndex = alreadyLoaded->second;
                if (existingIndex >= 0 && existingIndex < static_cast<int>(relevantBones.size())) {
                    relevantBones[existingIndex].weightCount += bone->mNumWeights;
                }
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
    finalBoneMatrixUpdated.assign(relevantBones.size(), 0);
    std::cout << "[BoneStats] unique weighted bone names = " << relevantBones.size() << std::endl;
    std::cout << "[BoneStats] boneInfoMap.size = " << boneNameToIndex.size() << std::endl;
    std::cout << "[BoneStats] finalBoneMatrices.size = " << finalBoneMatrices.size() << std::endl;
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

#pragma endregion

#pragma region Debug Controls
// Ű �Է����� ������ ����, �� �׽�Ʈ, ��/��ȣ ������ �ٲٴ� ���� �Լ� �����Դϴ�.

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

void Model::cycleElbowBendAxis() {
    const int nextAxis = (LocalAxisToIndex(bendConfig.leftElbow.axis) + 1) % 3;
    bendConfig.leftElbow.axis = LocalAxisFromIndex(nextAxis);
    bendConfig.rightElbow.axis = LocalAxisFromIndex(nextAxis);
    locomotionAxes.elbowBendAxis = nextAxis;
    std::cout << "[Locomotion] elbow bend axis=" << LocalAxisName(bendConfig.leftElbow.axis)
        << " (left/right config axis changed together; signs remain independent)" << std::endl;
    printFocusedJointDebug(BoneRole::LeftElbow);
    printFocusedJointDebug(BoneRole::RightElbow);
}

void Model::cycleLegSwingAxis() {
    locomotionAxes.legSwingAxis = (locomotionAxes.legSwingAxis + 1) % 3;
    printLocomotionDebug();
}

void Model::cycleKneeBendAxis() {
    const int nextAxis = (LocalAxisToIndex(bendConfig.leftKnee.axis) + 1) % 3;
    bendConfig.leftKnee.axis = LocalAxisFromIndex(nextAxis);
    bendConfig.rightKnee.axis = LocalAxisFromIndex(nextAxis);
    locomotionAxes.kneeBendAxis = nextAxis;
    std::cout << "[Locomotion] knee bend axis=" << LocalAxisName(bendConfig.leftKnee.axis)
        << " (left/right config axis changed together; signs remain independent)" << std::endl;
    printFocusedJointDebug(BoneRole::LeftKnee);
    printFocusedJointDebug(BoneRole::RightKnee);
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

void Model::cycleDebugJointTestMode() {
    const int mode = (static_cast<int>(debugJointTestMode) + 1) % (static_cast<int>(DebugJointTestMode::RightElbow_Z_Neg) + 1);
    debugJointTestMode = static_cast<DebugJointTestMode>(mode);

    BoneRole role = BoneRole::None;
    LocalAxis axis = LocalAxis::X;
    float sign = 1.0f;
    if (DebugJointModeToRoleAxisSign(debugJointTestMode, role, axis, sign)) {
        singleBoneTestRole = role;
        singleBoneTestAxis = LocalAxisToIndex(axis);
        std::cout << "[JointAxisTest] " << BoneRoleName(role)
            << " axis=" << LocalAxisName(axis)
            << " sign=" << (sign > 0.0f ? "+" : "-")
            << " angle=45deg" << std::endl;
        printFocusedJointDebug(role);
    }
    else {
        std::cout << "[JointAxisTest] None" << std::endl;
    }
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

#pragma endregion

#pragma region For Debug
// ��Ÿ�� ���� Ȯ�ΰ� PMX/��/�α� ������ ���� ����� ���� �Լ� �����Դϴ�.

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
        << ", debugJointTest=" << DebugJointTestModeName(debugJointTestMode)
        << ", shaderBones=" << locomotionBones.size()
        << ", primaryBones=" << primaryLocomotionBones.size() << std::endl;
    std::cout << "[AxisBasis] Local X=(1,0,0), Local Y=(0,1,0), Local Z=(0,0,1) in the selected bone local space" << std::endl;
    std::cout << "[AxisBasis] Positive/negative signs follow vmath::rotate right-hand rule and are applied as baseLocal * localRotation" << std::endl;
    std::cout << "[JointBendConfig] LeftElbow axis=" << LocalAxisName(bendConfig.leftElbow.axis)
        << " sign=" << bendConfig.leftElbow.sign
        << " minDeg=" << bendConfig.leftElbow.minAngleDeg
        << " maxDeg=" << bendConfig.leftElbow.maxAngleDeg
        << " | RightElbow axis=" << LocalAxisName(bendConfig.rightElbow.axis)
        << " sign=" << bendConfig.rightElbow.sign
        << " minDeg=" << bendConfig.rightElbow.minAngleDeg
        << " maxDeg=" << bendConfig.rightElbow.maxAngleDeg
        << std::endl;
    std::cout << "[JointBendConfig] LeftKnee axis=" << LocalAxisName(bendConfig.leftKnee.axis)
        << " sign=" << bendConfig.leftKnee.sign
        << " minDeg=" << bendConfig.leftKnee.minAngleDeg
        << " maxDeg=" << bendConfig.leftKnee.maxAngleDeg
        << " | RightKnee axis=" << LocalAxisName(bendConfig.rightKnee.axis)
        << " sign=" << bendConfig.rightKnee.sign
        << " minDeg=" << bendConfig.rightKnee.minAngleDeg
        << " maxDeg=" << bendConfig.rightKnee.maxAngleDeg
        << std::endl;
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
    if (singleBoneTestMode) {
        printFocusedJointDebug(singleBoneTestRole);
    }
    else {
        printFocusedJointDebug(BoneRole::LeftElbow);
        printFocusedJointDebug(BoneRole::RightElbow);
        printFocusedJointDebug(BoneRole::LeftKnee);
        printFocusedJointDebug(BoneRole::RightKnee);
    }
    std::cout << "[LocomotionDebug] snapshot end" << std::endl;
    std::cout << "============================================================" << std::endl;
    std::cout.flush();
}

#pragma endregion

#pragma region Locomotion Animation
// deltaTime ��� ���� phase/blend�� ���� �� ����� ����ϴ� �ִϸ��̼� �����Դϴ�.

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
    case BoneRole::RightElbow: {
        const float elbowPhaseOffset = (10.0f + (20.0f - 10.0f) * runBlend) * 0.01745329252f;
        float forward01 = bone.role == BoneRole::RightElbow
            ? 0.5f + 0.5f * std::sin(locomotionPhase + elbowPhaseOffset)
            : 0.5f + 0.5f * -std::sin(locomotionPhase + elbowPhaseOffset);
        forward01 = std::max(0.0f, std::min(1.0f, forward01));
        forward01 = forward01 * forward01 * (3.0f - 2.0f * forward01);
        const float maxElbowDeg = running ? 60.0f : 30.0f;
        const float bend = std::min(elbowBaseDeg + forward01 * elbowAddDeg, maxElbowDeg);
        const JointBendConfig& cfg = bone.hasBendOverride
            ? bone.bendOverride
            : (bone.role == BoneRole::LeftElbow ? bendConfig.leftElbow : bendConfig.rightElbow);
        static int elbowRuntimePrintsRemaining = 24;
        if (elbowRuntimePrintsRemaining > 0) {
            std::cout << "[ElbowRuntimeBend] target=" << bone.name
                << " role=" << BoneRoleName(bone.role)
                << " stage=" << bone.bendStage
                << " weight=" << bone.bendWeight
                << " axis=" << LocalAxisName(cfg.axis)
                << " sign=" << cfg.sign
                << " bendDeg=" << bend
                << " appliedDeg=" << (bend * bone.bendWeight)
                << " blend=" << blend
                << std::endl;
            --elbowRuntimePrintsRemaining;
        }
        return MakeJointBendRotation(cfg, bend * bone.bendWeight);
    }
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
        // Aemeath PMX debug result placeholder:
        // LeftKnee bend axis = locomotionAxes.kneeBendAxis, sign = locomotionAxes.leftKneeSign.
        // RightKnee bend axis = locomotionAxes.kneeBendAxis, sign = locomotionAxes.rightKneeSign.
        const float phase = locomotionPhase;
        const float swingLeft = std::max(0.0f, -std::sin(phase));
        const float swingRight = std::max(0.0f, std::sin(phase));
        const float downPulseLeft = std::pow(std::max(0.0f, std::cos(phase)), 2.0f);
        const float downPulseRight = std::pow(std::max(0.0f, -std::cos(phase)), 2.0f);
        const float swingBend = kneeBendDeg * (running ? 0.95f : 0.85f);
        const float downBend = running ? 14.0f : 6.0f;
        const float bend = bone.role == BoneRole::LeftKnee
            ? swingLeft * swingBend + downPulseLeft * downBend
            : swingRight * swingBend + downPulseRight * downBend;
        const JointBendConfig& cfg = bone.role == BoneRole::LeftKnee ? bendConfig.leftKnee : bendConfig.rightKnee;
        return MakeJointBendRotation(cfg, std::min(bend, kneeBendDeg) * blend);
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
    finalBoneMatrixUpdated.assign(relevantBones.size(), 0);
    updatedFinalBoneMatrixCount = 0;
    if (rootNode.name.empty()) return;
    computeFinalBoneMatricesRecursive(rootNode, vmath::mat4::identity());
}

void Model::computeFinalBoneMatricesRecursive(const SkeletonNode& node, const vmath::mat4& parentTransform) {
    vmath::mat4 offset = vmath::mat4::identity();
    bool debugJointRotationApplied = false;
    BoneRole debugJointRole = BoneRole::None;
    LocalAxis debugJointAxis = LocalAxis::X;
    float debugJointSignedAngle = 0.0f;
    const PrimaryLocomotionBone* appliedPrimaryBone = nullptr;
    if (!primaryLocomotionBones.empty()) {
        for (const PrimaryLocomotionBone& primaryBone : primaryLocomotionBones) {
            if (primaryBone.name == node.name) {
                appliedPrimaryBone = &primaryBone;
                if (singleBoneTestMode) {
                    BoneRole testRole = singleBoneTestRole;
                    int testAxis = singleBoneTestAxis;
                    float testAngle = singleBoneTestAngleDeg;
                    BoneRole explicitRole = BoneRole::None;
                    LocalAxis explicitAxis = LocalAxis::X;
                    float explicitSign = 1.0f;
                    if (DebugJointModeToRoleAxisSign(debugJointTestMode, explicitRole, explicitAxis, explicitSign)) {
                        testRole = explicitRole;
                        testAxis = LocalAxisToIndex(explicitAxis);
                        testAngle = 45.0f * explicitSign;
                    }
                    if (primaryBone.role == testRole) {
                        debugJointRotationApplied = explicitRole != BoneRole::None;
                        debugJointRole = primaryBone.role;
                        debugJointAxis = LocalAxisFromIndex(testAxis);
                        debugJointSignedAngle = testAngle;
                        if (testAxis == 0) {
                            offset = vmath::rotate(testAngle, 1.0f, 0.0f, 0.0f);
                        }
                        else if (testAxis == 1) {
                            offset = vmath::rotate(testAngle, 0.0f, 1.0f, 0.0f);
                        }
                        else {
                            offset = vmath::rotate(testAngle, 0.0f, 0.0f, 1.0f);
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
            if (boneIndex < static_cast<int>(finalBoneMatrixUpdated.size()) && finalBoneMatrixUpdated[boneIndex] == 0) {
                finalBoneMatrixUpdated[boneIndex] = 1;
                ++updatedFinalBoneMatrixCount;
            }
            if (debugJointRotationApplied) {
                static int elbowDebugPrintsRemaining = 24;
                if (elbowDebugPrintsRemaining > 0) {
                    std::cout << "[ElbowKneeDebug] selectedName=" << node.name
                        << " role=" << BoneRoleName(debugJointRole)
                        << " axis=" << LocalAxisName(debugJointAxis)
                        << " signedAngleDeg=" << debugJointSignedAngle
                        << " finalMatrixIndex=" << boneIndex
                        << " finalLocalTransformChanged=true"
                        << " globalTransformChanged=true"
                        << " finalBoneMatrixChanged=true"
                        << std::endl;
                    --elbowDebugPrintsRemaining;
                }
            }
            else if (appliedPrimaryBone && (appliedPrimaryBone->role == BoneRole::LeftElbow || appliedPrimaryBone->role == BoneRole::RightElbow)) {
                static int elbowRuntimeMatrixPrintsRemaining = 24;
                if (elbowRuntimeMatrixPrintsRemaining > 0) {
                    std::cout << "[ElbowRuntimeMatrix] selectedName=" << node.name
                        << " role=" << BoneRoleName(appliedPrimaryBone->role)
                        << " stage=" << appliedPrimaryBone->bendStage
                        << " finalMatrixIndex=" << boneIndex
                        << " finalLocalTransformChanged=true"
                        << " globalTransformChanged=true"
                        << " finalBoneMatrixChanged=true"
                        << std::endl;
                    --elbowRuntimeMatrixPrintsRemaining;
                }
            }
        }
    }

    for (const SkeletonNode& child : node.children) {
        computeFinalBoneMatricesRecursive(child, globalTransform);
    }
}

#pragma endregion

#pragma region Rendering Resources
// �� ��� ���ε�, �ؽ�ó �ε�, �ٴ� mesh ����/������ ���ҽ��� �ٷ�� �����Դϴ�.

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
        std::cout << "============================================================" << std::endl;
        std::cout << "[SkinningUpload] shader bone matrix count=" << boneCount << std::endl;
        printBoneMatrixUpdateStats();
        printCriticalBoneDiagnostics();
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

#pragma endregion
