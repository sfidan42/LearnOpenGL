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

struct DirLight
{
    vec3 direction; // This should be the direction TO the light
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

in vec3 textureDir;

uniform DirLight sunLight;
uniform samplerCube cubemap;

out vec4 FragColor;

void main()
{
    vec3 envColor = texture(cubemap, textureDir).rgb;

    vec3 normalDir = normalize(textureDir);

    // Use the direction AS the target.
    // If direction is (0, 1, 0), the sun is at the top.
    vec3 sunDir = normalize(sunLight.direction);

    float cosTheta = max(dot(normalDir, sunDir), 0.0);

    // Create the Sun Disk and Atmosphere Glow
    // pow() sharpens the highlight. High power = smaller, sharper sun.
    float sunGlow = pow(cosTheta, 64.0);       // Broad glow around the sun
    float sunDisk = pow(cosTheta, 2048.0);     // The actual bright sun circle

    // Combine colors
    // We add the sunlight (diffuse color) multiplied by our glow factors
    vec3 sunEffect = (sunGlow * sunLight.diffuse * 0.5) + (sunDisk * sunLight.specular);

    // Final color combines environment and sun effects
    vec3 finalColor = (envColor * sunLight.diffuse) + sunEffect;

    FragColor = vec4(finalColor, 1.0);
}
