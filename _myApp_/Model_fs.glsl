#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_alpha1;
uniform vec3 materialDiffuse;
uniform float materialOpacity;
uniform bool useDiffuseTexture;
uniform bool useAlphaTexture;
uniform bool flipDiffuseTextureV;
uniform bool flipAlphaTextureV;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;
uniform float ambientStrength;
uniform float specularStrength;
uniform float shininess;
uniform bool useFirstPersonClip;
uniform float firstPersonClipMaxY;

void main()
{
    if (useFirstPersonClip && FragPos.y > firstPersonClipMaxY) {
        discard;
    }

    vec2 diffuseUV = flipDiffuseTextureV ? vec2(TexCoords.x, 1.0 - TexCoords.y) : TexCoords;
    vec2 alphaUV = flipAlphaTextureV ? vec2(TexCoords.x, 1.0 - TexCoords.y) : TexCoords;
    vec4 baseColor = vec4(materialDiffuse, materialOpacity);

    if (useDiffuseTexture) {
        baseColor *= texture(texture_diffuse1, diffuseUV);
    }
    if (useAlphaTexture) {
        vec4 alphaMask = texture(texture_alpha1, alphaUV);
        baseColor.a *= min(alphaMask.r, alphaMask.a);
    }

    if (baseColor.a < 0.1) {
        discard;
    }

    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);

    vec3 ambient = ambientStrength * lightColor;
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = specularStrength * spec * lightColor;

    vec3 litColor = (ambient + diffuse + specular) * baseColor.rgb;
    FragColor = vec4(litColor, baseColor.a);
}
