#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;
layout (location = 2) in vec3 aNormal;
layout (location = 3) in ivec4 aBoneIDs;
layout (location = 4) in vec4 aWeights;

out vec2 TexCoords;
out vec3 FragPos;
out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform bool useSkinning;
uniform int boneCount;
uniform samplerBuffer boneMatricesTex;

mat4 FetchBoneMatrix(int boneID)
{
    int baseIndex = boneID * 4;
    return mat4(
        texelFetch(boneMatricesTex, baseIndex + 0),
        texelFetch(boneMatricesTex, baseIndex + 1),
        texelFetch(boneMatricesTex, baseIndex + 2),
        texelFetch(boneMatricesTex, baseIndex + 3)
    );
}

void main()
{
    mat4 skinMatrix = mat4(1.0);
    if (useSkinning) {
        skinMatrix = mat4(0.0);
        float totalWeight = 0.0;
        for (int i = 0; i < 4; ++i) {
            int boneID = aBoneIDs[i];
            float weight = aWeights[i];
            if (boneID >= 0 && boneID < boneCount && weight > 0.0) {
                skinMatrix += FetchBoneMatrix(boneID) * weight;
                totalWeight += weight;
            }
        }
        if (totalWeight <= 0.0) {
            skinMatrix = mat4(1.0);
        } else if (totalWeight < 1.0) {
            skinMatrix += mat4(1.0) * (1.0 - totalWeight);
        }
    }

    vec4 skinnedPos = skinMatrix * vec4(aPos, 1.0);
    vec3 skinnedNormal = mat3(skinMatrix) * aNormal;

    TexCoords = aTexCoords;
    FragPos = vec3(model * skinnedPos);
    Normal = mat3(transpose(inverse(model))) * skinnedNormal;
    gl_Position = projection * view * model * skinnedPos;
}
