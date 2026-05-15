#pragma once

#include <vector>
#include <sb7.h>
#include <vmath.h>
#include "ModelTypes.h"

// Mesh는 하나의 Assimp mesh를 OpenGL VAO/VBO/EBO와 텍스처 목록으로 보관합니다.
// 머리카락, 얼굴, 몸체처럼 같은 모델 안의 개별 파트를 독립적으로 그립니다.
class Mesh {
public:
    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;
    std::vector<Texture> textures;
    vmath::vec3 diffuseColor;
    float opacity = 1.0f;
    bool hasDiffuseTexture = false;
    bool hasAlphaTexture = false;
    bool flipTextureV = false;
    bool drawBackFacesFirst = false;
    bool alphaCutout = false;

    Mesh(std::vector<Vertex> vertices, std::vector<GLuint> indices, std::vector<Texture> textures,
         vmath::vec3 diffuseColor, float opacity, bool flipTextureV, bool drawBackFacesFirst, bool alphaCutout);
    void draw(GLuint shaderProgram);
    bool isTransparent() const;

private:
    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint EBO = 0;
    void setupMesh();
};