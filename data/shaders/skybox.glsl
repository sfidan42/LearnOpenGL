#shader vertex
#version 460 core

layout (location = 0) in vec3 aPos; // vertex position

uniform mat4 projection; // projection matrix
uniform mat4 view; // view matrix

out vec3 textureDir; // output direction vector for texture sampling

void main()
{
    textureDir = aPos; // use vertex position as direction vector
    vec4 pos = projection * view * vec4(aPos, 1.0); // transform vertex position
    gl_Position = pos.xyww; // set gl_Position with w component for depth correction
}

#shader fragment
#version 460 core

in vec3 textureDir; // direction vector representing a 3D texture coordinate

uniform samplerCube cubemap; // cubemap texture sampler
uniform vec3 viewPos;

out vec4 FragColor;

void main()
{
    FragColor = texture(cubemap, textureDir);
}
