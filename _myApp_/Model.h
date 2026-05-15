#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <sb7.h>
#include <vmath.h>
#include <assimp/scene.h>
#include "BoneTypes.h"
#include "LocomotionTypes.h"
#include "Mesh.h"
#include "ModelDrawParams.h"
#include "ModelTypes.h"
#include "SkeletonTypes.h"

// Model은 PMX/FBX 등 외부 모델 파일을 Assimp로 읽고 mesh/texture/bone 데이터를 관리합니다.
// 렌더링과 절차적 보행 구현은 아직 Model.cpp에 남아 있지만, 공용 타입 선언은 별도 헤더로 분리했습니다.
class Model {
public:
    std::vector<Texture> textures_loaded;
    std::vector<Mesh> meshes;
    std::vector<BoneData> relevantBones;
    std::string directory;
    GLuint shaderProgram = 0;
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
    void draw(const ModelDrawParams& params);
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