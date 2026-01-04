#shader vertex
#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in mat4 aInstanceMatrix;

uniform mat4 projection;
uniform mat4 view;

out vec3 FragPos;
out vec2 TexCoord;
out mat3 TBN;

void main()
{
    FragPos = vec3(aInstanceMatrix * vec4(aPos, 1.0));
    TexCoord = aTexCoord;

    // Normal matrix for instancing
    mat3 normalMatrix = mat3(transpose(inverse(aInstanceMatrix)));

    vec3 N = normalize(normalMatrix * aNormal);
    vec3 T = normalize(normalMatrix * aTangent);

    // Re-orthogonalize tangent
    T = normalize(T - dot(T, N) * N);

    vec3 B = cross(N, T);

    TBN = mat3(T, B, N);

    gl_Position = projection * view * vec4(FragPos, 1.0);
}

#shader fragment
#version 460 core

// ================= BINDLESS TEXTURES EXTENSION =================
#extension GL_ARB_bindless_texture : require

// ================= INPUTS =================
in vec3 FragPos;
in vec2 TexCoord;
in mat3 TBN;

out vec4 FragColor;

// ================= BINDLESS TEXTURE HANDLES =================
// Texture handles stored in SSBOs for unlimited textures
layout(std430, binding = 2) buffer DiffuseTextureHandles {
    sampler2D diffuseTextures[];
};

layout(std430, binding = 3) buffer SpecularTextureHandles {
    sampler2D specularTextures[];
};

layout(std430, binding = 4) buffer NormalTextureHandles {
    sampler2D normalTextures[];
};

uniform int u_numDiffuseTextures;
uniform int u_numSpecularTextures;
uniform int u_numNormalTextures;

float shininess = 32;

// ================= LIGHTS =================
struct DirLight
{
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight
{
    vec3 position;
    float constant;
    vec3 ambient;
    float linear;
    vec3 diffuse;
    float quadratic;
    vec3 specular;
    float _pad1;
};

struct SpotLight
{
    vec3 position;
    float cutOff;
    vec3 direction;
    float outerCutOff;
    vec3 ambient;
    float constant;
    vec3 diffuse;
    float linear;
    vec3 specular;
    float quadratic;
};

uniform DirLight sunLight;

// SSBOs for dynamic number of lights
layout(std430, binding = 0) buffer PointLightBuffer {
    PointLight pointLights[];
};

layout(std430, binding = 1) buffer SpotLightBuffer {
    SpotLight spotLights[];
};

uniform int u_numPointLights;
uniform int u_numSpotLights;

uniform vec3 viewPos;

// ================= FUNCTION PROTOTYPES =================
vec3 GetNormal();
vec3 GetDiffuseColor();
vec3 GetSpecularColor();

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir);

// ================= MAIN =================
void main()
{
    vec3 norm = GetNormal();
    vec3 viewDir = normalize(viewPos - FragPos);

    vec3 result = vec3(0.0);

    result += CalcDirLight(sunLight, norm, viewDir);

    for (int i = 0; i < u_numPointLights; i++)
        result += CalcPointLight(pointLights[i], norm, FragPos, viewDir);

    for (int i = 0; i < u_numSpotLights; i++)
        result += CalcSpotLight(spotLights[i], norm, FragPos, viewDir);

    // Gamma correction
    result = pow(result, vec3(1.0 / 2.2));

    FragColor = vec4(result, 1.0);
}

// ================= MATERIAL HELPERS =================

vec3 GetNormal()
{
    if (u_numNormalTextures <= 0)
        return normalize(TBN[2]); // world-space normal from vertex shader

    // Sample and average all normal maps (typically there's only one)
    vec3 normalMap = vec3(0.0);
    for (int i = 0; i < u_numNormalTextures; i++)
        normalMap += texture(normalTextures[i], TexCoord).rgb;

    normalMap /= float(u_numNormalTextures);
    normalMap = normalMap * 2.0 - 1.0; // [0,1] → [-1,1]

    return normalize(TBN * normalMap);
}

vec3 GetDiffuseColor()
{
    if (u_numDiffuseTextures <= 0)
        return vec3(1.0);

    vec3 color = vec3(0.0);
    for (int i = 0; i < u_numDiffuseTextures; i++)
        color += texture(diffuseTextures[i], TexCoord).rgb;

    return color / float(u_numDiffuseTextures);
}

vec3 GetSpecularColor()
{
    if (u_numSpecularTextures <= 0)
        return vec3(0.0);

    vec3 color = vec3(0.0);
    for (int i = 0; i < u_numSpecularTextures; i++)
        color += texture(specularTextures[i], TexCoord).rgb;

    return color / float(u_numSpecularTextures);
}

// ================= LIGHT CALCULATIONS =================

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(-light.direction);

    float diff = max(dot(normal, lightDir), 0.0);

    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);

    vec3 diffuseColor  = GetDiffuseColor();
    vec3 specularColor = GetSpecularColor();

    vec3 ambient  = light.ambient  * diffuseColor;
    vec3 diffuse  = light.diffuse  * diff * diffuseColor;
    vec3 specular = light.specular * spec * specularColor;

    return ambient + diffuse + specular;
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);

    float diff = max(dot(normal, lightDir), 0.0);

    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), shininess);

    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * distance * distance);

    vec3 diffuseColor  = GetDiffuseColor();
    vec3 specularColor = GetSpecularColor();

    vec3 ambient  = light.ambient  * diffuseColor;
    vec3 diffuse  = light.diffuse  * diff * diffuseColor;
    vec3 specular = light.specular * spec * specularColor;

    return (ambient + diffuse + specular) * attenuation;
}

vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);

    float diff = max(dot(normal, lightDir), 0.0);

    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);

    float distance = length(light.position - fragPos);
    float attenuation =
        1.0 / (light.constant +
        light.linear * distance +
        light.quadratic * distance * distance);

    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);

    vec3 diffuseColor  = GetDiffuseColor();
    vec3 specularColor = GetSpecularColor();

    vec3 ambient  = light.ambient  * diffuseColor;
    vec3 diffuse  = light.diffuse  * diff * diffuseColor;
    vec3 specular = light.specular * spec * specularColor;

    return (ambient + diffuse + specular) * attenuation * intensity;
}
