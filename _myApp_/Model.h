#pragma once

#include <vector>
#include <string>
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
    bool floorReady = false;
    float modelMinY = 0.0f;
    float modelMaxY = 0.0f;
    bool hasModelBounds = false;

    void init(const std::string& objFilePath, const std::string& vsPath, const std::string& fsPath);

    // 시간(회전용)과 화면 비율(원근투영용)을 매개변수로 받도록 수정
    void draw(float currentTime, float aspect, const vmath::vec3& characterPosition, float characterYawDegrees, float cameraYawDegrees, float cameraPitchDegrees, float cameraDistance, bool firstPersonCamera);

private:
    void loadModel(const std::string& path);
    void processNode(aiNode* node, const aiScene* scene);
    Mesh processMesh(aiMesh* mesh, const aiScene* scene);
    void processSkeletonData(const aiScene* scene);
    std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName);
    void setupFloor();
    void drawFloor(GLuint shaderProgram);
};
