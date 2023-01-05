#version 330 core

in vec2 texcoord;
uniform vec4 u_diffuse_color;
uniform sampler2D tex0; // texture unit from C++
out vec4 FragColor; // final output

void main()
{
    // modulate texture with material color, including transparency
    FragColor = u_diffuse_color * texture(tex0,texcoord); 
}