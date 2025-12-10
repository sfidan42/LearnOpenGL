#shader vertex
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in mat4 instanceModel;

uniform mat4 projection;
uniform mat4 view;

out vec2 TexCoord;
out vec3 Normal;
out vec3 FragPos;

void main()
{
    gl_Position = projection * view * instanceModel * vec4(aPos, 1.0);
    TexCoord = aTexCoord;
    Normal = mat3(transpose(inverse(instanceModel))) * aNormal;
    FragPos = (instanceModel * vec4(aPos, 1.0)).xyz;
}

#shader fragment
#version 330 core

in vec2 TexCoord;
uniform sampler2D texSampler;

uniform vec3 lightPos;
uniform vec3 lightColor;

in vec3 FragPos;
in vec3 Normal;
out vec4 FragColor;

void main()
{
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lightColor;
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    vec3 textureColor = texture(texSampler, TexCoord).rgb;
    vec3 result = (ambient + diffuse) * textureColor;
    FragColor = vec4(result, 1.0);
}
