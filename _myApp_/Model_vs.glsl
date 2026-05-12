#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;
layout (location = 2) in vec3 aNormal;

out vec2 TexCoords;
out vec3 FragPos;
out vec3 Normal;

// C++에서 넘겨줄 MVP 행렬들
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    TexCoords = aTexCoords;
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    
    // MVP 행렬을 정점의 원래 위치에 곱해서 화면의 올바른 위치로 변환합니다.
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}