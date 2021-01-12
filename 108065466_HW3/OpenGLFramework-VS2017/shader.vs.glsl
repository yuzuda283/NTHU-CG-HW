#version 330

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec3 aNormal;
layout (location = 3) in vec2 aTexCoord;

out vec2 texCoord;

uniform mat4 um4p;	// projection matrix
uniform mat4 um4v;	// camera viewing transformation matrix
uniform mat4 um4m;	// rotation matrix




out vec3 vertex_normal;
out vec3 FragPos;
out vec3 vertex_color;

void main() 
{
	//texCoord = (aTexCoord.x,1.0- aTexCoord.y);



	gl_Position = um4p * um4v * um4m * vec4(aPos, 1.0);
	vec4 fragPos = um4v * um4m *vec4(aPos.x, aPos.y, aPos.z, 1.0);
	FragPos =fragPos.xyz;	vertex_normal = mat3(transpose(inverse(um4v * um4m))) * aNormal;
	texCoord = aTexCoord;
}
