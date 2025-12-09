#shader vertex
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 3) in mat4 instanceModel;

out vec2 TexCoord;

void main()
{
    gl_Position = instanceModel * vec4(aPos, 1.0);
    TexCoord = aTexCoord;
}

#shader fragment
#version 330 core

in vec2 TexCoord;
uniform sampler2D texSampler;

out vec4 FragColor;

void main()
{
    FragColor = texture(texSampler, TexCoord);
}
