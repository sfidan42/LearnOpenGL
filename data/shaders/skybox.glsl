#shader vertex
#version 460 core

layout (location = 0) in vec3 aPos; // vertex position

uniform mat4 projection; // projection matrix
uniform mat4 view; // view matrix
uniform float scaleFactor; // scale factor for skybox size

out vec3 textureDir; // output direction vector for texture sampling

void main()
{
    textureDir = aPos; // use vertex position as direction vector
    vec4 pos = projection * view * vec4(aPos * scaleFactor, 1.0);
    gl_Position = pos.xyww; // set gl_Position with w component for depth correction
}

#shader fragment
#version 460 core

// ================= BINDLESS TEXTURES EXTENSION =================
#extension GL_ARB_bindless_texture : require

struct DirLightComponent
{
    vec3 direction;
    float pad0;

    vec3 ambient;
    float pad1;

    vec3 diffuse;
    float pad2;

    vec3 specular;
    float pad3;

    mat4 lightSpaceMatrix;
    sampler2DShadow shadowMap;
    float pad4[2];
};

// SSBO for dynamic number of directional lights
layout(std430, binding = 5) buffer DirLightBuffer {
    DirLightComponent dirLights[];
};

uniform int u_numDirLights;

in vec3 textureDir;

uniform samplerCube cubemap;

out vec4 FragColor;

void main()
{
    vec3 envColor = texture(cubemap, textureDir).rgb;
    vec3 normalDir = normalize(textureDir);

    // Accumulate lighting from all directional lights
    vec3 finalColor = vec3(0.0);

    for (int i = 0; i < u_numDirLights; i++)
    {
        DirLightComponent sunLight = dirLights[i];

        // If direction is (0, -1, 0), the sun is at the top.
        vec3 sunDir = normalize(-sunLight.direction);

        float cosTheta = max(dot(normalDir, sunDir), 0.0);

        // Create the Sun Disk and Atmosphere Glow
        // pow() sharpens the highlight. High power = smaller, sharper sun.
        float sunGlow = pow(cosTheta, 64.0);       // Broad glow around the sun
        float sunDisk = pow(cosTheta, 2048.0);     // The actual bright sun circle

        // Combine colors
        // We add the sunlight (diffuse color) multiplied by our glow factors
        vec3 sunEffect = (sunGlow * sunLight.diffuse * 0.5) + (sunDisk * sunLight.specular);

        // Accumulate lighting from this directional light
        finalColor += (envColor * sunLight.diffuse) + sunEffect;
    }

    // If no directional lights, just show the environment map
    if (u_numDirLights == 0)
        finalColor = envColor;

    FragColor = vec4(finalColor, 1.0);
}
