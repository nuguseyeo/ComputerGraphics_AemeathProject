#include "Mesh.h"

#include <cstddef>

#pragma region Mesh
// 단일 mesh의 OpenGL 버퍼 생성, 머티리얼 uniform 설정, draw call을 담당하는 구역입니다.

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

#pragma endregion