#version 330 core

out vec4 FragColor;
in vec3 vertex_color;

void main() {
	FragColor = vec4(vertex_color, 1.0f);
}
