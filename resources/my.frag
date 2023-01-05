#version 330 core

in vec4 color; // smooth defaultnì
// flat in vec4 color; // bere first nebo last vertex barvu
//uniform vec4 color;

out vec4 FragColor; // Final output

void main() {
	FragColor = color;
}
