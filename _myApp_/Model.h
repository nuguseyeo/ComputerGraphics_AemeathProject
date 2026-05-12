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

struct Vertex {
    vmath::vec3 position;
    vmath::vec2 texCoord;
    vmath::vec3 normal;
};

struct Texture {
    GLuint id;
    std::string type;
    std::string path;
};

struct Bounds3 {
    vmath::vec3 min;
    vmath::vec3 max;
    bool valid = false;
};

struct BoneData {
    std::string name;
    int sourceIndex;
    vmath::mat4 offsetMatrix;
};

class Mesh {
public:
    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;
    std::vector<Texture> textures;
    vmath::vec3 diffuseColor;
    float opacity;
    bool hasDiffuseTexture;
    bool hasAlphaTexture;
    bool usesDiffuseAlpha;
    bool flipDiffuseTextureV;
    bool flipAlphaTextureV;
    bool drawBackFacesFirst;
    bool alphaCutout;

    Mesh(std::vector<Vertex> vertices, std::vector<GLuint> indices, std::vector<Texture> textures,
         vmath::vec3 diffuseColor, float opacity, bool usesDiffuseAlpha, bool flipDiffuseTextureV, bool flipAlphaTextureV, bool drawBackFacesFirst, bool alphaCutout);
    void draw(GLuint shaderProgram);
    bool isTransparent() const;

private:
    GLuint VAO, VBO, EBO;
    void setupMesh();
};

class Model {
public:
    std::vector<Texture> textures_loaded;
    std::vector<Mesh> meshes;
    std::vector<BoneData> relevantBones;
    std::string directory;
    GLuint shaderProgram;
    GLuint floorVAO = 0, floorVBO = 0, floorEBO = 0;
    GLuint floorTexture = 0;
    bool floorReady = false;
    float modelMinY = 0.0f;
    float modelMaxY = 0.0f;
    bool hasModelBounds = false;
    int upAxis = 1;
    float importScale = 1.0f;
    float importGroundOffset = 0.0f;

    void init(const std::string& objFilePath, const std::string& vsPath, const std::string& fsPath);
    void draw(float currentTime, float aspect, const vmath::vec3& characterPosition, float characterYawDegrees, float cameraYawDegrees, float cameraPitchDegrees, float cameraDistance, bool firstPersonCamera);

private:
    void loadModel(const std::string& path);
    void processNode(aiNode* node, const aiScene* scene);
    Mesh processMesh(aiMesh* mesh, const aiScene* scene);
    void configureImportTransform(const aiScene* scene);
    vmath::vec3 transformImportedPosition(const aiVector3D& position) const;
    vmath::vec3 transformImportedNormal(const aiVector3D& normal) const;
    void processSkeletonData(const aiScene* scene);
    std::vector<Texture> loadFallbackMaterialTextures(aiMaterial* mat, std::string typeName);
    std::vector<Texture> loadMaterialTextures(aiMaterial* mat, const aiScene* scene, aiTextureType type, std::string typeName);
    void setupFloor();
    void drawFloor(GLuint shaderProgram);
};
