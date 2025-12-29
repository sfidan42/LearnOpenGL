#shader vertex
#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in mat4 aInstanceMatrix;

uniform mat4 projection;
uniform mat4 view;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;

void main()
{
    FragPos = vec3(aInstanceMatrix * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(aInstanceMatrix))) * aNormal;
    TexCoord = aTexCoord;
    gl_Position = projection * view * aInstanceMatrix * vec4(aPos, 1.0);
}

#shader fragment
#version 460 core

// ================= INPUTS =================
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

out vec4 FragColor;

// ================= TEXTURE ARRAYS =================
uniform sampler2D u_diffuseTextures[16];
uniform int u_numDiffuseTextures;

uniform sampler2D u_specularTextures[16];
uniform int u_numSpecularTextures;

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
    float linear;
    float quadratic;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct SpotLight
{
    vec3 position;
    vec3 direction;

    float cutOff;
    float outerCutOff;

    float constant;
    float linear;
    float quadratic;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

#define MAX_POINT_LIGHTS 16
#define MAX_SPOT_LIGHTS 16

uniform DirLight sunLight;

uniform PointLight pointLights[MAX_POINT_LIGHTS];
uniform int u_numPointLights;

uniform SpotLight spotLights[MAX_SPOT_LIGHTS];
uniform int u_numSpotLights;

uniform vec3 viewPos;

// ================= FUNCTION PROTOTYPES =================
vec3 GetDiffuseColor();
vec3 GetSpecularColor();

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir);

// ================= MAIN =================
void main()
{
    vec3 norm = normalize(Normal);
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

vec3 GetDiffuseColor()
{
    if (u_numDiffuseTextures <= 0)
        return vec3(1.0);

    vec3 color = vec3(0.0);
    for (int i = 0; i < u_numDiffuseTextures; i++)
        color += texture(u_diffuseTextures[i], TexCoord).rgb;

    return color / float(u_numDiffuseTextures);
}

vec3 GetSpecularColor()
{
    if (u_numSpecularTextures <= 0)
        return vec3(0.0);

    vec3 color = vec3(0.0);
    for (int i = 0; i < u_numSpecularTextures; i++)
        color += texture(u_specularTextures[i], TexCoord).rgb;

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
