#version 330

in vec2 texCoord;
in vec3 vertex_normal;
in vec3 FragPos;

uniform vec3 Ka;
uniform vec3 Kd;
uniform vec3 Ks;
uniform float shininess;
uniform vec3 La = vec3(0.15f, 0.15f, 0.15f);
uniform vec3 Ld1;
uniform vec3 Ld2;
uniform vec3 Ld3;
uniform vec3 lightPos1;uniform vec3 lightPos2;uniform vec3 lightPos3;uniform int lightIdx;
uniform float spotCutoff;

uniform mat4 um4v;
uniform sampler2D tex;
//uniform sampler2D texture0;

out vec4 fragColor;



vec3 directlight(){
	vec3 N = normalize(vertex_normal);
	//vec3 L = lightPos - FragPos;
	vec3 L = lightPos1 - FragPos;
	L = normalize(L); //lightDir
	vec3 V = normalize(-FragPos);	//camera vector
	vec3 R = reflect(-L, N); //reflect
	vec3 ambient = La * Ka;
	vec3 diffuse = Ld1 * Kd * max(dot(N, L), 0);
	vec3 specular = Ks * pow(max(dot(R, V), 0), shininess);
	vec3 color = ambient + diffuse + specular;
	return color;
}	

vec3 pointlight(){
	vec3 N = normalize(vertex_normal);
	//vec3 L = lightPos - FragPos;
	vec4 lightInView = um4v * vec4(lightPos2,1.0);
	vec3 S = normalize(lightInView.xyz);
	vec3 V = normalize(-FragPos);
	vec3 H = normalize(S + V);	
	vec3 pos = lightInView.xyz - FragPos;
	vec3 L = normalize(pos);
	L = normalize(L); //lightDir
	//vec3 R = reflect(-L, N); //reflect
	float dist = length(lightPos2 - FragPos);
	float cal = 0.01 + 0.8 * dist + 0.1 * dist * dist;
	float factor = 1/(cal);
	vec3 ambient = La * Ka;
	vec3 diffuse = Ld2 * Kd * max(dot(N, L), 0);
	vec3 specular = Ks * pow(max(dot(N, H), 0), shininess);
	vec3 color = ambient + factor*diffuse + factor*specular;
	return color;
	}

vec3 spotlight(){
	vec3 N = normalize(vertex_normal);
	//vec3 L = lightPos - FragPos;
	vec4 lightInView = um4v * vec4(lightPos3,1.0);
	vec3 S = normalize(lightInView.xyz);
	vec3 V = normalize(-FragPos);
	vec3 H = normalize(S + V);	
	vec3 pos = lightInView.xyz - FragPos;
	vec3 L = normalize(pos);
	L = normalize(L); //lightDir
	//vec3 R = reflect(-L, N); //reflect
	float dist = length(lightPos3 - FragPos);
	float cal = 0.05 + 0.3 * dist + 0.6 * dist * dist;
	float factor = 1/(cal/5);

	vec4 spotdirection = vec4(0.0f, 0.0f,-1.0f,1.0f);
	vec3 direction = normalize((transpose(inverse(um4v))*spotdirection).xyz);
	float calculate = dot(-L, direction);

	if(spotCutoff <= degrees(acos(calculate)))
	{
	vec3 ambient = La * Ka;
	vec3 diffuse = Ld3 * Kd * max(dot(N, L), 0);
	vec3 specular = Ks * pow(max(dot(N, H), 0), shininess);
	vec3 color = ambient ;
	//+ factor*diffuse + factor*specular;
	return color;
	}
	else
	{
	float effect = pow(max(calculate, 0), 50);
	//float effect =0.0;
	vec3 ambient = La * Ka;
	vec3 diffuse = Ld3 * Kd * max(dot(N, L), 0);
	vec3 specular = Ks * pow(max(dot(N, H), 0), shininess);
	vec3 color = ambient + factor*effect*diffuse + factor*effect*specular;
	return color;
	}
}



// [TODO] passing texture from main.cpp
// Hint: sampler2D

void main() {
	vec3 color = vec3(0, 0, 0);
	if(lightIdx %3 == 0)
	{
		color += directlight();
	}
	else if(lightIdx % 3 == 1)
	{
		color += pointlight();
	}
	else if(lightIdx % 3 == 2)
	{
		color += spotlight();
	}
	vec4 glColor = vec4 (color ,1.0);
	fragColor = vec4(texture(tex, texCoord).rgb, 1.0) * glColor;
	//vec4 textcolor = texture2D(,texCoord)
	// [TODO] sampleing from texture
	// Hint: texture
}
