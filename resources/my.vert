#version 330 core

layout (location = 0) in vec3 aPos; // Positions/Coordinates
layout (location = 1) in vec3 aColor; // Color RGB
layout (location = 2) in vec2 aTexCoord; // Texture coordinates
layout (location = 3) in vec3 aNormal; // Normal vector

uniform mat4 uP_m;
uniform mat4 uV_m;
uniform mat4 uM_m;

// Outputs colors in RGBA
out vec4 FragColor;

// Exports the current position to the Fragment Shader
out vec3 pos;
// Exports the color to the Fragment Shader
out vec3 color;
// Exports texture coordinates to the Fragment Shader
out vec2 texCoord;
// Exports the normal to the Fragment Shader
out vec3 Normal;

void main() {
	// Outputs coordinates of all vertices
	pos = uP_m * uV_m * uM_m * vec4(aPos, 1.0f);
	color = aColor;
	texCoord = aTexCoord;
	Normal = aNormal;
}
