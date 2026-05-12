#include "Model.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cmath>
#include <windows.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#ifndef GL_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#endif
#ifndef GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#endif

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
    hasModelBounds = false;
    modelMinY = 0.0f;
    modelMaxY = 0.0f;

    loadModel(objFilePath);
    setupFloor();
}

void Model::draw(float currentTime, float aspect, const vmath::vec3& characterPosition, float characterYawDegrees, float cameraYawDegrees, float cameraPitchDegrees, float cameraDistance, bool firstPersonCamera) {
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glUseProgram(shaderProgram);

    const float degToRad = 0.0174532925f;
    const float yawRad = cameraYawDegrees * degToRad;
    const float pitchRad = cameraPitchDegrees * degToRad;
    const float horizontalDistance = std::cos(pitchRad) * cameraDistance;
    const float modelHeight = hasModelBounds ? (modelMaxY - modelMinY) : 0.0f;
    const float localEyeY = hasModelBounds ? (modelMinY + modelHeight * 0.965f) : 8.0f;
    const float eyeHeight = localEyeY - 2.0f;
    const float firstPersonEyeForwardOffset = 0.16f;

    vmath::vec3 target = characterPosition + vmath::vec3(0.0f, 4.0f, 0.0f);
    vmath::vec3 eye = target + vmath::vec3(
        -std::sin(yawRad) * horizontalDistance,
        std::sin(pitchRad) * cameraDistance,
        std::cos(yawRad) * horizontalDistance
    );

    if (firstPersonCamera) {
        vmath::vec3 horizontalView(std::sin(yawRad), 0.0f, -std::cos(yawRad));
        eye = characterPosition + vmath::vec3(0.0f, eyeHeight, 0.0f) + horizontalView * firstPersonEyeForwardOffset;
        vmath::vec3 lookDirection(
            std::sin(yawRad) * std::cos(pitchRad),
            std::sin(pitchRad),
            -std::cos(yawRad) * std::cos(pitchRad)
        );
        target = eye + lookDirection;
    }

    if (eye[1] < 0.5f) {
        eye[1] = 0.5f;
    }

    vmath::mat4 projection = vmath::perspective(firstPersonCamera ? 32.0f : 45.0f, aspect, firstPersonCamera ? 0.08f : 0.1f, 100.0f);
    vmath::mat4 view = vmath::lookat(
        eye,
        target,
        vmath::vec3(0.0f, 1.0f, 0.0f)
    );
    vmath::mat4 model_matrix = vmath::translate(characterPosition[0], characterPosition[1] - 2.0f, characterPosition[2])
        * vmath::rotate(characterYawDegrees, 0.0f, 1.0f, 0.0f)
        * vmath::scale(1.0f);

    GLuint projLoc = glGetUniformLocation(shaderProgram, "projection");
    GLuint viewLoc = glGetUniformLocation(shaderProgram, "view");
    GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");

    glUniformMatrix4fv(projLoc, 1, GL_FALSE, projection);
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, view);
    glUniform3f(glGetUniformLocation(shaderProgram, "lightPos"), 18.0f, 28.0f, 12.0f);
    glUniform3f(glGetUniformLocation(shaderProgram, "viewPos"), eye[0], eye[1], eye[2]);
    glUniform3f(glGetUniformLocation(shaderProgram, "lightColor"), 1.0f, 0.94f, 0.82f);
    glUniform1f(glGetUniformLocation(shaderProgram, "ambientStrength"), 0.28f);
    glUniform1f(glGetUniformLocation(shaderProgram, "specularStrength"), 0.35f);
    glUniform1f(glGetUniformLocation(shaderProgram, "shininess"), 32.0f);

    glUniform1i(glGetUniformLocation(shaderProgram, "useFirstPersonClip"), 0);
    drawFloor(shaderProgram);
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, model_matrix);
    const bool drawCharacterMesh = !firstPersonCamera || cameraPitchDegrees < -6.0f;
    glUniform1i(glGetUniformLocation(shaderProgram, "useFirstPersonClip"), firstPersonCamera ? 1 : 0);
    const float firstPersonChestClipY = hasModelBounds ? characterPosition[1] + modelMinY + modelHeight * 0.78f - 2.0f : eye[1] - 1.35f;
    glUniform1f(glGetUniformLocation(shaderProgram, "firstPersonClipMaxY"), firstPersonChestClipY);

    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);

    if (drawCharacterMesh) {
        for (unsigned int i = 0; i < meshes.size(); i++) {
            if (!meshes[i].isTransparent()) meshes[i].draw(shaderProgram);
        }
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    if (drawCharacterMesh) {
        for (unsigned int i = 0; i < meshes.size(); i++) {
            if (meshes[i].isTransparent()) meshes[i].draw(shaderProgram);
        }
    }
    glDepthMask(GL_TRUE);
}

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

    processNode(scene->mRootNode, scene);
}

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