#version 330 core

in vec2 texcoord;

uniform sampler2D tex0; // texture unit from C++

out vec4 FragColor; // final output

void main()
{
    FragColor = texture(tex0, texcoord);
}
