#shader vertex
#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 4) in mat4 aInstanceMatrix;

uniform mat4 lightSpaceMatrix;

void main()
{
    gl_Position = lightSpaceMatrix * aInstanceMatrix * vec4(aPos, 1.0);
}

#shader fragment
#version 460 core

void main()
{
    // gl_FragDepth is written automatically
}
