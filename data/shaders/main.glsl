#shader vertex
#version 460 core

// ================= ATTRIBUTES =================
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in mat4 aInstanceMatrix;

// ================= UNIFORMS =================
uniform mat4 projection;
uniform mat4 view;

// ================= OUTPUTS =================
out vec3 FragPos;
out vec2 TexCoord;
out mat3 TBN;

// ================= VERTEX SHADER =================
void main()
{
    // Transform position to world space
    FragPos = vec3(aInstanceMatrix * vec4(aPos, 1.0));
    TexCoord = aTexCoord;

    // Calculate TBN matrix for normal mapping
    mat3 normalMatrix = mat3(transpose(inverse(aInstanceMatrix)));
    vec3 N = normalize(normalMatrix * aNormal);
    vec3 T = normalize(normalMatrix * aTangent);
    T = normalize(T - dot(T, N) * N); // Re-orthogonalize tangent
    vec3 B = cross(N, T);
    TBN = mat3(T, B, N);

    // Final clip space position
    gl_Position = projection * view * vec4(FragPos, 1.0);
}

#shader fragment
#version 460 core

// ================= EXTENSIONS =================
#extension GL_ARB_bindless_texture : require

// ================= INPUTS =================
in vec3 FragPos;
in vec2 TexCoord;
in mat3 TBN;

out vec4 FragColor;

// ================= CONSTANTS =================
const vec3 MISSING_TEXTURE_COLOR = vec3(1.0, 0.0, 1.0); // Magenta
const vec3 DEFAULT_SPECULAR_COLOR = vec3(0.5);          // Gray
const float SHININESS = 32.0;
const float GAMMA = 2.2;

// ================= TEXTURE SSBOs =================
layout(std430, binding = 0) readonly buffer DiffuseTextures {
    sampler2D diffuse[];
};
layout(std430, binding = 1) readonly buffer SpecularTextures {
    sampler2D specular[];
};
layout(std430, binding = 2) readonly buffer NormalTextures {
    sampler2D normal[];
};

uniform int u_numDiffuse;
uniform int u_numSpecular;
uniform int u_numNormal;

// ================= LIGHT STRUCTURES =================
struct DirLight {
    vec3 direction;      float pad0;
    vec3 ambient;        float pad1;
    vec3 diffuse;        float pad2;
    vec3 specular;       float pad3;
    mat4 lightSpaceMatrix;
    sampler2DShadow shadowMap;
    float pad4[2];
};

struct PointLight {
    vec3 position;       float constant;
    vec3 ambient;        float linear;
    vec3 diffuse;        float quadratic;
    vec3 specular;       float farPlane;
    mat4 shadowMatrices[6];
    samplerCube shadowMap;
    float _pad[3];
};

struct SpotLight {
    vec3 position;       float cutOff;
    vec3 direction;      float outerCutOff;
    vec3 ambient;        float constant;
    vec3 diffuse;        float linear;
    vec3 specular;       float quadratic;
    mat4 lightSpaceMatrix;
    sampler2DShadow shadowMap;
    float _pad[2];
};

// ================= LIGHT SSBOs =================
layout(std430, binding = 3) readonly buffer PointLights {
    PointLight lights[];
} pointLights;
layout(std430, binding = 4) readonly buffer SpotLights {
    SpotLight lights[];
} spotLights;
layout(std430, binding = 5) readonly buffer DirLights {
    DirLight lights[];
} dirLights;

// ================= LIGHT COUNTS =================
uniform int u_numPointLights;
uniform int u_numSpotLights;
uniform int u_numDirLights;

// ================= CAMERA =================
uniform vec3 viewPos;

// ================= FUNCTION PROTOTYPES =================
// Material functions
vec3 getNormal();
vec3 getDiffuseColor();
vec3 getSpecularColor();

// Shadow functions
float calcDirShadow(DirLight light, vec3 fragPos, vec3 normal, vec3 lightDir);
float calcPointShadow(PointLight light, vec3 fragPos, vec3 normal);
float calcSpotShadow(SpotLight light, vec3 fragPos, vec3 normal, vec3 lightDir);

// Lighting functions
vec3 calcDirLight(DirLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
vec3 calcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
vec3 calcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir);

// ================= PCF SAMPLING =================
const vec2 POISSON_DISK[4] = vec2[](
vec2(-0.94201624, -0.39906216),
vec2( 0.94558609, -0.76890725),
vec2(-0.09418410, -0.92938870),
vec2( 0.34495938,  0.29387760)
);

// ================= MAIN FUNCTION =================
void main()
{
    vec3 normal = getNormal();
    vec3 viewDir = normalize(viewPos - FragPos);

    vec3 result = vec3(0.0);

    // Accumulate lighting from all light types
    for (int i = 0; i < u_numDirLights; i++)
    result += calcDirLight(dirLights.lights[i], normal, FragPos, viewDir);

    for (int i = 0; i < u_numPointLights; i++)
    result += calcPointLight(pointLights.lights[i], normal, FragPos, viewDir);

    for (int i = 0; i < u_numSpotLights; i++)
    result += calcSpotLight(spotLights.lights[i], normal, FragPos, viewDir);

    // Gamma correction
    result = pow(result, vec3(1.0 / GAMMA));
    FragColor = vec4(result, 1.0);
}

// ================= MATERIAL FUNCTIONS =================

// Get normal from normal map or vertex normal
vec3 getNormal()
{
    if (u_numNormal <= 0)
    return normalize(TBN[2]); // Use vertex normal

    // Average all normal maps (typically one)
    vec3 normalMap = vec3(0.0);
    for (int i = 0; i < u_numNormal; i++)
    normalMap += texture(normal[i], TexCoord).rgb;

    normalMap /= float(u_numNormal);
    normalMap = normalMap * 2.0 - 1.0; // Convert [0,1] to [-1,1]

    return normalize(TBN * normalMap);
}

// Get diffuse color from texture(s)
vec3 getDiffuseColor()
{
    if (u_numDiffuse <= 0)
    return MISSING_TEXTURE_COLOR; // Error color

    vec3 color = vec3(0.0);
    for (int i = 0; i < u_numDiffuse; i++)
    color += texture(diffuse[i], TexCoord).rgb;

    return color / float(u_numDiffuse);
}

// Get specular color from texture(s)
vec3 getSpecularColor()
{
    if (u_numSpecular <= 0)
    return DEFAULT_SPECULAR_COLOR; // Default gray

    vec3 color = vec3(0.0);
    for (int i = 0; i < u_numSpecular; i++)
    color += texture(specular[i], TexCoord).rgb;

    return color / float(u_numSpecular);
}

// ================= SHADOW FUNCTIONS =================

// Directional light shadow calculation
float calcDirShadow(DirLight light, vec3 fragPos, vec3 normal, vec3 lightDir)
{
    // Transform to light space
    vec4 fragPosLightSpace = light.lightSpaceMatrix * vec4(fragPos, 1.0);
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5; // Convert to [0,1] range

    // Check if outside shadow map
    if (projCoords.z > 1.0 ||
    projCoords.x < 0.0 || projCoords.x > 1.0 ||
    projCoords.y < 0.0 || projCoords.y > 1.0)
    return 0.0;

    // Calculate bias based on surface angle
    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.001);
    float currentDepth = projCoords.z - bias;

    // PCF sampling for soft shadows
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(light.shadowMap, 0);

    for (int i = 0; i < 4; i++) {
        vec2 offset = POISSON_DISK[i] * texelSize;
        shadow += texture(light.shadowMap, vec3(projCoords.xy + offset, currentDepth));
    }

    return 1.0 - (shadow / 4.0);
}

// Point light shadow calculation (omnidirectional)
float calcPointShadow(PointLight light, vec3 fragPos, vec3 normal)
{
    vec3 fragToLight = fragPos - light.position;
    float currentDepth = length(fragToLight);
    vec3 lightDir = normalize(light.position - fragPos);

    // Slope-based bias
    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.0005);

    // Normal offset to prevent shadow acne
    vec3 samplePos = fragToLight + (normal * 0.15);

    // 3D PCF sampling
    float shadow = 0.0;
    float samples = 2.0;
    float offset = 0.01;

    for (float x = -offset; x <= offset; x += offset / samples) {
        for (float y = -offset; y <= offset; y += offset / samples) {
            for (float z = -offset; z <= offset; z += offset / samples) {
                float closestDepth = texture(light.shadowMap, samplePos + vec3(x, y, z)).r;
                closestDepth *= light.farPlane; // Convert to linear depth
                shadow += (currentDepth - bias > closestDepth) ? 1.0 : 0.0;
            }
        }
    }

    float totalSamples = pow(samples * 2.0 + 1.0, 3.0);
    return shadow / totalSamples;
}

// Spotlight shadow calculation
float calcSpotShadow(SpotLight light, vec3 fragPos, vec3 normal, vec3 lightDir)
{
    // Check if fragment is in spotlight cone
    vec3 toLight = light.position - fragPos;
    float theta = dot(normalize(toLight), normalize(-light.direction));

    if (theta < light.outerCutOff)
    return 1.0; // Fully shadowed (outside cone)

    // Transform to light space
    vec4 fragPosLightSpace = light.lightSpaceMatrix * vec4(fragPos, 1.0);

    if (fragPosLightSpace.w <= 0.0)
    return 1.0; // Behind light

    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    projCoords.xy = clamp(projCoords.xy, 0.0, 1.0); // Avoid wrapping

    if (projCoords.z > 1.0)
    return 1.0; // Outside far plane

    float bias = max(0.002 * (1.0 - dot(normal, lightDir)), 0.0005);
    float currentDepth = projCoords.z - bias;

    // Hardware PCF
    return 1.0 - texture(light.shadowMap, vec3(projCoords.xy, currentDepth));
}

// ================= LIGHTING FUNCTIONS =================

// Calculate directional light contribution
vec3 calcDirLight(DirLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(-light.direction);

    // Diffuse component
    float diff = max(dot(normal, lightDir), 0.0);

    // Blinn-Phong specular (more efficient than Phong)
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), SHININESS);

    // Material colors
    vec3 diffuseColor = getDiffuseColor();
    vec3 specularColor = getSpecularColor();

    // Lighting components
    vec3 ambient = light.ambient * diffuseColor;
    vec3 diffuse = light.diffuse * diff * diffuseColor;
    vec3 specular = light.specular * spec * specularColor;

    // Apply shadows
    float shadow = calcDirShadow(light, fragPos, normal, lightDir);
    float visibility = 1.0 - shadow;

    return ambient + visibility * (diffuse + specular);
}

// Calculate point light contribution
vec3 calcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);

    // Diffuse component
    float diff = max(dot(normal, lightDir), 0.0);

    // Blinn-Phong specular
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), SHININESS);

    // Distance attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant +
    light.linear * distance +
    light.quadratic * distance * distance);

    // Material colors
    vec3 diffuseColor = getDiffuseColor();
    vec3 specularColor = getSpecularColor();

    // Lighting components
    vec3 ambient = light.ambient * diffuseColor;
    vec3 diffuse = light.diffuse * diff * diffuseColor;
    vec3 specular = light.specular * spec * specularColor;

    // Apply shadows
    float shadow = calcPointShadow(light, fragPos, normal);
    float visibility = 1.0 - shadow;

    return (ambient + visibility * (diffuse + specular)) * attenuation;
}

// Calculate spotlight contribution
vec3 calcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);

    // Back-face culling
    float facingLight = max(dot(normal, lightDir), 0.0);
    if (facingLight <= 0.0)
    return vec3(0.0);

    // Check spotlight cone
    vec3 spotDir = normalize(-light.direction);
    float theta = dot(lightDir, spotDir);

    if (theta < light.outerCutOff)
    return vec3(0.0);

    // Soft edge between inner and outer cone
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);

    // Diffuse component
    float diff = facingLight;

    // Blinn-Phong specular
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), SHININESS);

    // Distance attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant +
    light.linear * distance +
    light.quadratic * distance * distance);

    // Material colors
    vec3 diffuseColor = getDiffuseColor();
    vec3 specularColor = getSpecularColor();

    // Lighting components
    vec3 ambient = light.ambient * diffuseColor;
    vec3 diffuse = light.diffuse * diff * diffuseColor;
    vec3 specular = light.specular * spec * specularColor;

    // Apply shadows
    float shadow = calcSpotShadow(light, fragPos, normal, lightDir);
    float visibility = 1.0 - shadow;

    return (ambient + visibility * (diffuse + specular)) * attenuation * intensity;
}
