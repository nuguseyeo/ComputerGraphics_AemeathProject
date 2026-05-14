#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <iostream>
#include <sb7.h>
#include <vmath.h>
#include <shader.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// 1. 버텍스 구조체
struct Vertex {
    vmath::vec3 position;
    vmath::vec2 texCoord;
    vmath::vec3 normal;
    int boneIDs[4] = { -1, -1, -1, -1 };
    float boneWeights[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
};

// 2. 텍스처 구조체 (Assimp에서 읽어온 텍스처 정보 저장)
struct Texture {
    GLuint id;
    std::string type;
    std::string path;
};

struct BoneData {
    std::string name;
    int meshIndex;
    int sourceIndex;
    unsigned int weightCount;
    vmath::mat4 offsetMatrix;
    int animationIndex;
    bool excludedFromLocomotion;
};

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

enum class BoneSide {
    Center,
    Left,
    Right,
    Unknown
};

enum class LocomotionState {
    Idle,
    Walk,
    Run
};

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

enum class LocomotionDebugLevel {
    PelvisOnly,
    LeftLegOnly,
    LowerBodyOnly,
    FullBody
};

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

enum class LocalAxis {
    X,
    Y,
    Z
};

struct JointBendConfig {
    LocalAxis axis = LocalAxis::X;
    float sign = 1.0f;
    float minAngleDeg = 0.0f;
    float maxAngleDeg = 30.0f;
};

struct LimbBendConfig {
    JointBendConfig leftKnee = { LocalAxis::X, 1.0f, 0.0f, 30.0f };
    JointBendConfig rightKnee = { LocalAxis::X, 1.0f, 0.0f, 30.0f };
    JointBendConfig leftElbow = { LocalAxis::X, -1.0f, 0.0f, 60.0f };
    JointBendConfig rightElbow = { LocalAxis::X, -1.0f, 0.0f, 60.0f };
};

struct LocomotionBone {
    int boneIndex;
    BoneGroup group;
    BoneSide side;
    bool appliesOffset;
};

struct PrimaryLocomotionBone {
    std::string name;
    int skinningIndex;
    BoneRole role;
    JointBendConfig bendOverride;
    bool hasBendOverride = false;
    float bendWeight = 1.0f;
    int bendStage = 0;
};

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

struct SkeletonNode {
    std::string name;
    vmath::mat4 baseTransform;
    std::vector<SkeletonNode> children;
};

// 3. 개별 파트(머리카락, 얼굴 등)를 담당할 Mesh 클래스
class Mesh {
public:
    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;
    std::vector<Texture> textures;
    vmath::vec3 diffuseColor;
    float opacity;
    bool hasDiffuseTexture;
    bool hasAlphaTexture;
    bool flipTextureV;
    bool drawBackFacesFirst;
    bool alphaCutout;

    Mesh(std::vector<Vertex> vertices, std::vector<GLuint> indices, std::vector<Texture> textures,
         vmath::vec3 diffuseColor, float opacity, bool flipTextureV, bool drawBackFacesFirst, bool alphaCutout);
    void draw(GLuint shaderProgram);
    bool isTransparent() const;

private:
    GLuint VAO, VBO, EBO;
    void setupMesh();
};

// 4. 전체 모델을 관리할 Model 클래스
class Model {
public:
    std::vector<Texture> textures_loaded; // 이미 로드된 텍스처 캐싱 (중복 로드 방지)
    std::vector<Mesh> meshes;
    std::vector<BoneData> relevantBones;
    std::string directory; // 모델 파일이 있는 디렉토리 경로
    GLuint shaderProgram;
    GLuint floorVAO = 0, floorVBO = 0, floorEBO = 0;
    GLuint floorTexture = 0;
    GLuint boneMatrixBuffer = 0;
    GLuint boneMatrixTexture = 0;
    GLuint skeletonLineVAO = 0;
    GLuint skeletonLineVBO = 0;
    GLsizei skeletonLineVertexCount = 0;
    bool floorReady = false;
    float modelMinY = 0.0f;
    float modelMaxY = 0.0f;
    bool hasModelBounds = false;
    bool proceduralLocomotionEnabled = true;

    void init(const std::string& objFilePath, const std::string& vsPath, const std::string& fsPath);

    // 시간(회전용)과 화면 비율(원근투영용)을 매개변수로 받도록 수정
    void draw(float currentTime, float deltaTime, float aspect, const vmath::vec3& characterPosition, float characterYawDegrees, float cameraYawDegrees, float cameraPitchDegrees, float cameraDistance, bool firstPersonCamera, bool hasMovementInput, float movementSpeedScale);
    void setProceduralLocomotionEnabled(bool enabled);
    bool isProceduralLocomotionEnabled() const { return proceduralLocomotionEnabled; }
    void setLocomotionDebugLevel(LocomotionDebugLevel level);
    void setBindPoseMode(bool enabled);
    void setSkeletonDebugDraw(bool enabled);
    void setSingleBoneTestMode(bool enabled);
    void adjustShoulderCorrection(float deltaDegrees);
    void cycleShoulderCorrectionAxis();
    void cycleArmSwingAxis();
    void cycleElbowBendAxis();
    void cycleLegSwingAxis();
    void cycleKneeBendAxis();
    void flipShoulderCorrectionSign();
    void flipLeftShoulderDownSign();
    void flipRightShoulderDownSign();
    void cycleSingleBoneTestRole();
    void cycleSingleBoneTestAxis();
    void cycleDebugJointTestMode();
    void adjustSingleBoneTestAngle(float deltaDegrees);
    void setRunModeEnabled(bool enabled);
    void printLocomotionDebug() const;

private:
    SkeletonNode rootNode;
    vmath::mat4 globalInverseTransform = vmath::mat4::identity();
    std::unordered_map<std::string, int> boneNameToIndex;
    std::vector<LocomotionBone> locomotionBones;
    std::vector<PrimaryLocomotionBone> primaryLocomotionBones;
    std::vector<vmath::mat4> finalBoneMatrices;
    SkeletonBoneMap boneMap;
    ProceduralLocomotionSettings locomotionSettings;
    LocomotionAxisSettings locomotionAxes;
    LimbBendConfig bendConfig;
    float locomotionPhase = 0.0f;
    float locomotionBlend = 0.0f;
    LocomotionState locomotionState = LocomotionState::Idle;
    LocomotionDebugLevel locomotionDebugLevel = LocomotionDebugLevel::FullBody;
    bool hasRequiredBoneRoles = false;
    bool bindPoseMode = false;
    bool skeletonDebugDraw = false;
    bool singleBoneTestMode = false;
    float shoulderCorrectionDeg = 35.0f;
    int shoulderCorrectionAxis = 2;
    int shoulderCorrectionSign = -1;
    BoneRole singleBoneTestRole = BoneRole::LeftUpperArm;
    int singleBoneTestAxis = 0;
    float singleBoneTestAngleDeg = 30.0f;
    DebugJointTestMode debugJointTestMode = DebugJointTestMode::None;
    float walkTime = 0.0f;
    float walkBlend = 0.0f;
    float runBlend = 0.0f;
    bool forceRunAnimation = false;
    unsigned long long skinningVertexCount = 0;
    unsigned long long zeroWeightVertexCount = 0;
    unsigned long long badWeightVertexCount = 0;
    unsigned long long droppedInfluenceCount = 0;
    unsigned long long boneIdsOverUploadedRange = 0;
    int maxBoneIdUsedByVertices = -1;
    unsigned long long influenceHistogram[5] = {};
    float minWeightSum = 9999.0f;
    float maxWeightSum = 0.0f;
    double totalWeightSum = 0.0;
    std::vector<unsigned char> finalBoneMatrixUpdated;
    unsigned int updatedFinalBoneMatrixCount = 0;

    void loadModel(const std::string& path);
    void processNode(aiNode* node, const aiScene* scene);
    Mesh processMesh(aiMesh* mesh, const aiScene* scene);
    void processSkeletonData(const aiScene* scene);
    SkeletonNode copySkeletonNode(aiNode* node) const;
    void buildPrimaryLocomotionRig();
    void collectSkeletonNodeNames(const SkeletonNode& node, std::vector<std::string>& names) const;
    const PrimaryLocomotionBone* findPrimaryBoneByRole(BoneRole role) const;
    const SkeletonNode* findSkeletonNodeByName(const SkeletonNode& node, const std::string& name, const SkeletonNode** parent) const;
    void printSelectedBoneInfo(BoneRole role) const;
    void printFullSkeletonTable(const aiScene* scene) const;
    void printSkinningStats() const;
    void printBoneLimitSummary() const;
    void printGLLimits() const;
    void printCriticalBoneDiagnostics() const;
    void printBoneMatrixUpdateStats() const;
    void autoConfigureJointBendsFromSkeleton();
    void printJointBasisDiagnostics() const;
    void printFocusedJointDebug(BoneRole role) const;
    void setupSkeletonDebugLines();
    void collectSkeletonDebugLines(const SkeletonNode& node, const vmath::mat4& parentTransform, std::vector<float>& lineVertices) const;
    void drawSkeletonDebug(GLuint shaderProgram);
    void updateLocomotion(float deltaTime, bool hasMovementInput, float movementSpeedScale);
    void computeFinalBoneMatrices();
    void computeFinalBoneMatricesRecursive(const SkeletonNode& node, const vmath::mat4& parentTransform);
    vmath::mat4 proceduralOffsetForPrimaryBone(const PrimaryLocomotionBone& bone) const;
    void uploadBoneMatrices(GLuint shaderProgram) const;
    std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName);
    void setupFloor();
    void drawFloor(GLuint shaderProgram);
};
