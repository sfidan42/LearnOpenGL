#shader vertex
#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 4) in mat4 aInstanceMatrix;

out vec3 FragPos;

void main()
{
    FragPos = vec3(aInstanceMatrix * vec4(aPos, 1.0));
    gl_Position = vec4(FragPos, 1.0);
}

#shader geometry
#version 460 core
layout (triangles) in;
layout (triangle_strip, max_vertices = 18) out;

uniform mat4 shadowMatrices[6];
uniform vec3 lightPos;

in vec3 FragPos[];

out vec4 GeoFragPos;

void main()
{
    for (int face = 0; face < 6; ++face)
    {
        gl_Layer = face; // built-in variable that specifies which face we render to
        for (int i = 0; i < 3; ++i)
        {
            GeoFragPos = vec4(FragPos[i].xyz, 1.0);
            gl_Position = shadowMatrices[face] * GeoFragPos;
            EmitVertex();
        }
        EndPrimitive();
    }
}

#shader fragment
#version 460 core

in vec4 GeoFragPos;

uniform vec3 lightPos;
uniform float farPlane;

void main()
{
    float lightDistance = length(GeoFragPos.xyz - lightPos);
    gl_FragDepth = lightDistance / farPlane;
}
