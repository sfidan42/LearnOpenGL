#shader vertex
#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in mat4 instanceModel;

uniform mat4 projection;
uniform mat4 view;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;

void main()
{
    gl_Position = projection * view * instanceModel * vec4(aPos, 1.0);
    TexCoord = aTexCoord;
    Normal = mat3(transpose(inverse(instanceModel))) * aNormal;
    FragPos = (instanceModel * vec4(aPos, 1.0)).xyz;
}

#shader fragment
#version 460 core

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};

struct Light {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

uniform Material material;
uniform sampler2D texSampler;
uniform Light light;
uniform vec3 viewPos;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

out vec4 FragColor;

void main()
{
    // ambient
    vec3 ambient  = light.ambient * material.ambient;

    // diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(light.position - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse  = light.diffuse * (diff * material.diffuse);

    // specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = light.specular * (spec * material.specular);

    // combine results
    vec4 lightResult = vec4(ambient + diffuse + specular, 1.0f);
    vec4 textureColor = texture(texSampler, TexCoord);
    FragColor = lightResult * textureColor;
}
