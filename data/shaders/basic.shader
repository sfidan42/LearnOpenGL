#shader vertex
#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoord = aTexCoord;

    gl_Position = projection * view * model * vec4(aPos, 1.0);
}

#shader fragment
#version 460 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

out vec4 FragColor;

// Multiple diffuse textures
uniform sampler2D u_diffuseTextures[16];
uniform int u_numDiffuseTextures;

void main()
{
    vec4 albedo = vec4(1.0);
    if(u_numDiffuseTextures > 0)
    {
        albedo = vec4(0.0);
        for(int i = 0; i < u_numDiffuseTextures; ++i)
        {
            albedo += texture(u_diffuseTextures[i], TexCoord);
        }
        albedo /= float(u_numDiffuseTextures);
    }

    // Output the texture color directly (no lighting)
    FragColor = albedo;
}
