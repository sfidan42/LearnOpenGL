#shader vertex
#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in mat4 aInstanceMatrix;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 lightSpaceMatrix;

out vec3 FragPos;
out vec2 TexCoord;
out mat3 TBN;
out vec4 FragPosLightSpace;

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

    FragPosLightSpace = lightSpaceMatrix * vec4(FragPos, 1.0);

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
in vec4 FragPosLightSpace;

out vec4 FragColor;

// ================= SHADOW MAP =================
uniform sampler2DShadow shadowMap;

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
float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir);

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, float shadow);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir);

// ================= MAIN =================
void main()
{
    vec3 norm = GetNormal();
    vec3 viewDir = normalize(viewPos - FragPos);

    vec3 result = vec3(0.0);

    // Calculate shadow for directional light
    vec3 lightDir = normalize(-sunLight.direction);
    float shadow = ShadowCalculation(FragPosLightSpace, norm, lightDir);

    result += CalcDirLight(sunLight, norm, viewDir, shadow);

    for (int i = 0; i < u_numPointLights; i++)
        result += CalcPointLight(pointLights[i], norm, FragPos, viewDir);

    for (int i = 0; i < u_numSpotLights; i++)
        result += CalcSpotLight(spotLights[i], norm, FragPos, viewDir);

    // Gamma correction
    result = pow(result, vec3(1.0 / 2.2));

    FragColor = vec4(result, 1.0);
}

// ================= MATERIAL HELPERS =================

const vec3 MISSING_TEXTURE_COLOR = vec3(1.0, 0.0, 1.0); // Magenta for missing textures (like Blender)

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
        return MISSING_TEXTURE_COLOR; // Show magenta for missing diffuse texture

    vec3 color = vec3(0.0);
    for (int i = 0; i < u_numDiffuseTextures; i++)
        color += texture(diffuseTextures[i], TexCoord).rgb;

    return color / float(u_numDiffuseTextures);
}

vec3 GetSpecularColor()
{
    if (u_numSpecularTextures <= 0)
        return vec3(0.5); // Gray fallback for missing specular (common default)

    vec3 color = vec3(0.0);
    for (int i = 0; i < u_numSpecularTextures; i++)
        color += texture(specularTextures[i], TexCoord).rgb;

    return color / float(u_numSpecularTextures);
}

// ================= SHADOW CALCULATION =================

// Poisson disk for soft shadow sampling
const vec2 poissonDisk[16] = vec2[](
    vec2(-0.94201624, -0.39906216),
    vec2(0.94558609, -0.76890725),
    vec2(-0.094184101, -0.92938870),
    vec2(0.34495938, 0.29387760),
    vec2(-0.91588581, 0.45771432),
    vec2(-0.81544232, -0.87912464),
    vec2(-0.38277543, 0.27676845),
    vec2(0.97484398, 0.75648379),
    vec2(0.44323325, -0.97511554),
    vec2(0.53742981, -0.47373420),
    vec2(-0.26496911, -0.41893023),
    vec2(0.79197514, 0.19090188),
    vec2(-0.24188840, 0.99706507),
    vec2(-0.81409955, 0.91437590),
    vec2(0.19984126, 0.78641367),
    vec2(0.14383161, -0.14100790)
);

float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
    // Perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    // Transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;

    // Check if fragment is outside the shadow map
    if (projCoords.z > 1.0)
        return 0.0;

    // Calculate bias based on surface angle
    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.001);

    float currentDepth = projCoords.z - bias;

    // PCF with Poisson sampling for soft shadows
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);

    for (int i = 0; i < 4; i++)
    {
        vec2 offset = poissonDisk[i] * texelSize * 2.0;
        shadow += texture(shadowMap, vec3(projCoords.xy + offset, currentDepth));
    }
    shadow /= 4.0;

    // Return shadow factor (0 = fully shadowed, 1 = fully lit)
    return 1.0 - shadow;
}

// ================= LIGHT CALCULATIONS =================

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, float shadow)
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

    // Apply shadow to diffuse and specular (ambient is not affected)
    float visibility = 1.0 - shadow;
    return ambient + visibility * (diffuse + specular);
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
