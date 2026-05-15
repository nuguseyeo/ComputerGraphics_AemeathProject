#pragma once

#include <string>
#include <sb7.h>
#include <vmath.h>

// Vertex는 GPU로 업로드되는 단일 정점 데이터입니다.
// 위치/UV/노멀과 최대 4개의 스키닝 본 인덱스/가중치를 함께 보관합니다.
struct Vertex {
    vmath::vec3 position;
    vmath::vec2 texCoord;
    vmath::vec3 normal;
    int boneIDs[4] = { -1, -1, -1, -1 };
    float boneWeights[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
};

// Texture는 Assimp 머티리얼에서 읽어온 텍스처와 OpenGL 핸들을 연결합니다.
struct Texture {
    GLuint id = 0;
    std::string type;
    std::string path;
};