#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTex;

uniform mat4 uP_m, uV_m, uM_m;

out VS_OUT
{
    vec2 texcoord;
} vs_out;

void main()
{
    // Outputs the positions/coordinates of all vertices
    gl_Position = uP_m * uV_m * uM_m * vec4(aPos, 1.0f);

    vs_out.texcoord = aTex;
}