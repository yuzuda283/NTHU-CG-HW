#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include<math.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "textfile.h"

#include "Vectors.h"
#include "Matrices.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#define PI 3.14159

#ifndef max
# define max(a,b) (((a)>(b))?(a):(b))
# define min(a,b) (((a)<(b))?(a):(b))
#endif

using namespace std;

// Default window size
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

bool mouse_pressed = false;
int starting_press_x = -1;
int starting_press_y = -1;

enum TransMode
{
	GeoTranslation = 0,
	GeoRotation = 1,
	GeoScaling = 2,
	ViewCenter = 3,
	ViewEye = 4,
	ViewUp = 5,
	Shine = 6,
	Changelightmode =7,
};

GLint iLocMVP;

vector<string> filenames; // .obj filename list

struct PhongMaterial
{
	Vector3 Ka;
	Vector3 Kd;
	Vector3 Ks;

};

typedef struct
{
	GLuint vao;
	GLuint vbo;
	GLuint vboTex;
	GLuint ebo;
	GLuint p_color;
	int vertex_count;
	GLuint p_normal;
	PhongMaterial material;
	int indexCount;
	GLuint m_texture;
} Shape;

struct model
{
	Vector3 position = Vector3(0, 0, 0);
	Vector3 scale = Vector3(1, 1, 1);
	Vector3 rotation = Vector3(0, 0, 0);	// Euler form

	vector<Shape> shapes;
};
vector<model> models;


struct camera
{
	Vector3 position;
	Vector3 center;
	Vector3 up_vector;
};
camera main_camera;


struct project_setting
{
	GLfloat nearClip, farClip;
	GLfloat fovy;
	GLfloat aspect;
	GLfloat left, right, top, bottom;
};
project_setting proj;


enum ProjMode
{
	Orthogonal = 0,
	Perspective = 1,
};
ProjMode cur_proj_mode = Orthogonal;
TransMode cur_trans_mode = GeoTranslation;

Matrix4 view_matrix;
Matrix4 project_matrix;

Shape quad;
Shape m_shpae;
int cur_idx = 0; // represent which model should be rendered now


static GLvoid Normalize(GLfloat v[3])
{
	GLfloat l;

	l = (GLfloat)sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
	v[0] /= l;
	v[1] /= l;
	v[2] /= l;
}

static GLvoid Cross(GLfloat u[3], GLfloat v[3], GLfloat n[3])
{

	n[0] = u[1] * v[2] - u[2] * v[1];
	n[1] = u[2] * v[0] - u[0] * v[2];
	n[2] = u[0] * v[1] - u[1] * v[0];
}







// [TODO] given a translation vector then output a Matrix4 (Translation Matrix)
Matrix4 translate(Vector3 vec)
{
	Matrix4 mat;

	/*
	mat = Matrix4(
		...
	);
	*/
	mat = Matrix4(
		1, 0, 0, vec.x,
		0, 1, 0, vec.y,
		0, 0, 1, vec.z,
		0, 0, 0, 1
	);

	return mat;
}

// [TODO] given a scaling vector then output a Matrix4 (Scaling Matrix)
Matrix4 scaling(Vector3 vec)
{
	Matrix4 mat;

	/*
	mat = Matrix4(
		...
	);
	*/
	mat = Matrix4(
		vec.x, 0, 0, 0,
		0, vec.y, 0, 0,
		0, 0, vec.z, 0,
		0, 0, 0, 1
	);

	return mat;
}


// [TODO] given a float value then ouput a rotation matrix alone axis-X (rotate alone axis-X)
Matrix4 rotateX(GLfloat val)
{
	Matrix4 mat;

	/*
	mat = Matrix4(
		...
	);
	*/
	val = val * PI / 180;

	mat = Matrix4(
		1, 0, 0, 0,
		0, cos(val), -sin(val), 0,
		0, sin(val), cos(val), 0,
		0, 0, 0, 1
	);

	return mat;
}

// [TODO] given a float value then ouput a rotation matrix alone axis-Y (rotate alone axis-Y)
Matrix4 rotateY(GLfloat val)
{
	Matrix4 mat;

	/*
	mat = Matrix4(
		...
	);
	*/
	val = val * PI / 180;

	mat = Matrix4(
		cos(val), 0, sin(val), 0,
		0, 1, 0, 0,
		-sin(val), 0, cos(val), 0,
		0, 0, 0, 1
	);

	return mat;
}

// [TODO] given a float value then ouput a rotation matrix alone axis-Z (rotate alone axis-Z)
Matrix4 rotateZ(GLfloat val)
{
	Matrix4 mat;

	/*
	mat = Matrix4(
		...
	);
	*/
	val = val * PI / 180;

	mat = Matrix4(
		cos(val), -sin(val), 0, 0,
		sin(val), cos(val), 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	);
	return mat;
}

Matrix4 rotate(Vector3 vec)
{
	return rotateX(vec.x)*rotateY(vec.y)*rotateZ(vec.z);
}


// [TODO] compute viewing matrix accroding to the setting of main_camera
void setViewingMatrix()
{
	// view_matrix[...] = ...
	Vector3 Z = (main_camera.center - main_camera.position);
	Z.normalize();
	// Side_direction
	Vector3 X = Z.cross(main_camera.up_vector);
	X.normalize();
	Vector3 Y = X.cross(Z).normalize();

	Matrix4 M, eye;

	M = Matrix4(
		X.x, X.y, X.z, 0,
		Y.x, Y.y, Y.z, 0,
		-Z.x, -Z.y, -Z.z, 0,
		0, 0, 0, 1
	);

	eye = Matrix4(
		1, 0, 0, -main_camera.position.x,
		0, 1, 0, -main_camera.position.y,
		0, 0, 1, -main_camera.position.z,
		0, 0, 0, 1
	);

	view_matrix = M * eye;
}

// [TODO] compute orthogonal projection matrix
void setOrthogonal()
{
	cur_proj_mode = Orthogonal;
	// project_matrix [...] = ...
	float M0 = 2 / (proj.right - proj.left);
	float M1 = 2 / (proj.top - proj.bottom);
	float M2 = -2 / (proj.farClip - proj.nearClip);
	float m1 = -1 * (proj.right + proj.left) / (proj.right - proj.left);
	float m2 = -1 * (proj.top + proj.bottom) / (proj.top - proj.bottom);
	float m3 = -1 * (proj.farClip + proj.nearClip) / (proj.farClip - proj.nearClip);
	project_matrix = Matrix4(
		M0, 0, 0, m1,
		0, M1, 0, m2,
		0, 0, M2, m3,
		0, 0, 0, 1
	);
}

// [TODO] compute persepective projection matrix
void setPerspective()
{
	cur_proj_mode = Perspective;
	// project_matrix [...] = ...
	float m00 = (1 / tan(proj.fovy * PI / 180 / 2)) / proj.aspect;
	float m11 = 1 / tan(proj.fovy * PI / 180 / 2);
	float m22 = (proj.nearClip + proj.farClip) / (proj.nearClip - proj.farClip);
	float m23 = 2 * proj.nearClip * proj.farClip / (proj.nearClip - proj.farClip);
	project_matrix = Matrix4(
		m00, 0, 0, 0,
		0, m11, 0, 0,
		0, 0, m22, m23,
		0, 0, -1, 0
	);
}


// Vertex buffers
GLuint VAO, VBO;

// Call back function for window reshape
void ChangeSize(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
	// [TODO] change your aspect ratio
	float aspect_ratio = (float)width / (float)height;

	if (cur_proj_mode == Perspective)
	{
		float m00 = (1 / tan(proj.fovy*PI / 180 / 2)) / aspect_ratio;
		float m11 = 1 / tan(proj.fovy*PI / 180 / 2);
		float m22 = (proj.nearClip + proj.farClip) / (proj.nearClip - proj.farClip);
		float m23 = 2 * proj.nearClip * proj.farClip / (proj.nearClip - proj.farClip);
		project_matrix = Matrix4(
			m00, 0, 0, 0,
			0, m11, 0, 0,
			0, 0, m22, m23,
			0, 0, -1, 0
		);
		if (cur_proj_mode == Orthogonal)
		{
			float M0 = 2 / (proj.right - proj.left);
			float M1 = 2 / (proj.top - proj.bottom);
			float M2 = -2 / (proj.farClip - proj.nearClip);
			float m1 = -1 * (proj.right + proj.left) / (proj.right - proj.left);
			float m2 = -1 * (proj.top + proj.bottom) / (proj.top - proj.bottom);
			float m3 = -1 * (proj.farClip + proj.nearClip) / (proj.farClip - proj.nearClip);
			project_matrix = Matrix4(
				M0 / aspect_ratio, 0, 0, 0,
				0, M1, 0, 0,
				0, 0, M2, m3,
				0, 0, 0, 1
			);
		}
	}
}


GLint iLocKa;
GLint iLocKd;
GLint iLocKs;
GLint iLocmv;
GLint iLocview;
GLint iLoccamera;
GLint iLocShininess;
GLint iLocLd1;
GLint iLocLd2;
GLint iLocLd3;
Vector3 Ld1 = Vector3(1, 1, 1);
Vector3 Ld2 = Vector3(1, 1, 1);
Vector3 Ld3 = Vector3(1, 1, 1);
GLint iLocLp1;
GLint iLocLp2;
GLint iLocLp3;
Vector3 Lp1 = Vector3(1.0f, 1.0f, 1.0f);
Vector3 Lp2 = Vector3(0.0f, 2.0f, 1.0f);
Vector3 Lp3 = Vector3(0.0f, 0.0f, 2.0f);
float shininess = 64.0f;
int perpex = 0;
GLuint iLocperpex;


GLuint iLocLightIdx;
int light_idx = 3;
GLuint iLocspotcutoff;
float spotcutoff = 30;

struct iLocLightInfo
{
	GLuint position;
	GLuint ambient;
	GLuint diffuse;
	GLuint specular;
	GLuint spotDirection;
	GLuint spotCutoff;
	GLuint spotExponent;
	GLuint constantAttenuation;
	GLuint linearAttenuation;
	GLuint quadraticAttenuation;
}iLocLightInfo[3];

struct LightInfo
{
	Vector4 position;
	Vector4 spotDirection;
	Vector4 ambient;
	Vector4 diffuse;
	Vector4 specular;
	float spotExponent;
	float spotCutoff;
	float constantAttenuation;
	float linearAttenuation;
	float quadraticAttenuation;
}lightInfo[3];

bool enableAmbient = true;
bool enableDiffuse = true;
bool enableSpecular = true;

//lightInfo[0].position = Vector4(3.0f, 3.0f, 3.0f, 0.0f);
//lightInfo[0].ambient = Vector4(0.15f, 0.15f, 0.15f, 1.0f);
//lightInfo[0].diffuse = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
//lightInfo[0].specular = Vector4(1.0f, 1.0f, 1.0f, 1.0f);


void updateLight()
{
	if (enableAmbient)
	{
		glUniform4f(iLocLightInfo[0].ambient, lightInfo[0].ambient.x, lightInfo[0].ambient.y, lightInfo[0].ambient.z, lightInfo[0].ambient.w);
		glUniform4f(iLocLightInfo[1].ambient, lightInfo[1].ambient.x, lightInfo[1].ambient.y, lightInfo[1].ambient.z, lightInfo[1].ambient.w);
		glUniform4f(iLocLightInfo[2].ambient, lightInfo[2].ambient.x, lightInfo[2].ambient.y, lightInfo[2].ambient.z, lightInfo[2].ambient.w);
	}
	else
	{
		float zeros[] = { 0.0f, 0.0f, 0.0f, 0.0f };
		glUniform4fv(iLocLightInfo[0].ambient, 1, zeros);
		glUniform4fv(iLocLightInfo[1].ambient, 1, zeros);
		glUniform4fv(iLocLightInfo[2].ambient, 1, zeros);
	}

	if (enableDiffuse)
	{
		glUniform4f(iLocLightInfo[0].diffuse, lightInfo[0].diffuse.x, lightInfo[0].diffuse.y, lightInfo[0].diffuse.z, lightInfo[0].diffuse.w);
		glUniform4f(iLocLightInfo[1].diffuse, lightInfo[1].diffuse.x, lightInfo[1].diffuse.y, lightInfo[1].diffuse.z, lightInfo[1].diffuse.w);
		glUniform4f(iLocLightInfo[2].diffuse, lightInfo[2].diffuse.x, lightInfo[2].diffuse.y, lightInfo[2].diffuse.z, lightInfo[2].diffuse.w);
	}
	else
	{
		float zeros[] = { 0.0f, 0.0f, 0.0f, 0.0f };
		glUniform4fv(iLocLightInfo[0].diffuse, 1, zeros);
		glUniform4fv(iLocLightInfo[1].diffuse, 1, zeros);
		glUniform4fv(iLocLightInfo[2].diffuse, 1, zeros);
	}

	if (enableSpecular)
	{
		glUniform4f(iLocLightInfo[0].specular, lightInfo[0].specular.x, lightInfo[0].specular.y, lightInfo[0].specular.z, lightInfo[0].specular.w);
		glUniform4f(iLocLightInfo[1].specular, lightInfo[1].specular.x, lightInfo[1].specular.y, lightInfo[1].specular.z, lightInfo[1].specular.w);
		glUniform4f(iLocLightInfo[2].specular, lightInfo[2].specular.x, lightInfo[2].specular.y, lightInfo[2].specular.z, lightInfo[2].specular.w);
	}
	else
	{
		float zeros[] = { 0.0f, 0.0f, 0.0f, 0.0f };
		glUniform4fv(iLocLightInfo[0].specular, 1, zeros);
		glUniform4fv(iLocLightInfo[1].specular, 1, zeros);
		glUniform4fv(iLocLightInfo[2].specular, 1, zeros);
	}


	glUniform4f(iLocLightInfo[0].position, lightInfo[0].position.x, lightInfo[0].position.y, lightInfo[0].position.z, lightInfo[0].position.w);

	glUniform4f(iLocLightInfo[1].position, lightInfo[1].position.x, lightInfo[1].position.y, lightInfo[1].position.z, lightInfo[1].position.w);
	glUniform1f(iLocLightInfo[1].constantAttenuation, lightInfo[1].constantAttenuation);
	glUniform1f(iLocLightInfo[1].linearAttenuation, lightInfo[1].linearAttenuation);
	glUniform1f(iLocLightInfo[1].quadraticAttenuation, lightInfo[1].quadraticAttenuation);

	glUniform4f(iLocLightInfo[2].position, lightInfo[2].position.x, lightInfo[2].position.y, lightInfo[2].position.z, lightInfo[2].position.w);
	glUniform4f(iLocLightInfo[2].spotDirection, lightInfo[2].spotDirection.x, lightInfo[2].spotDirection.y, lightInfo[2].spotDirection.z, lightInfo[2].spotDirection.w);
	glUniform1f(iLocLightInfo[2].spotExponent, lightInfo[2].spotExponent);
	glUniform1f(iLocLightInfo[2].spotCutoff, lightInfo[2].spotCutoff);
	glUniform1f(iLocLightInfo[2].constantAttenuation, lightInfo[2].constantAttenuation);
	glUniform1f(iLocLightInfo[2].linearAttenuation, lightInfo[2].linearAttenuation);
	glUniform1f(iLocLightInfo[2].quadraticAttenuation, lightInfo[2].quadraticAttenuation);
}

// Render function for display rendering
void RenderScene(void) {	
	// clear canvas
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	Matrix4 T, R, S;
	// [TODO] update translation, rotation and scaling
	T = translate(models[cur_idx].position);

	R = rotate(models[cur_idx].rotation);

	S = scaling(models[cur_idx].scale);

	Matrix4 MVP;
	GLfloat mvp[16];

	// [TODO] multiply all the matrix
	MVP = project_matrix * view_matrix * T * R * S;
	// [TODO] row-major ---> column-major

	mvp[0] = MVP[0];  mvp[4] = MVP[1];   mvp[8] = MVP[2];    mvp[12] = MVP[3];
	mvp[1] = MVP[4];  mvp[5] = MVP[5];   mvp[9] = MVP[6];    mvp[13] = MVP[7];
	mvp[2] = MVP[8];  mvp[6] = MVP[9];   mvp[10] = MVP[10];   mvp[14] = MVP[11];
	mvp[3] = MVP[12]; mvp[7] = MVP[13];  mvp[11] = MVP[14];   mvp[15] = MVP[15];


	// use uniform to send mvp to vertex shader
	//glUniform3f(glGetUniformLocation(p, "lightPos"), 1.0f, 1.0f, 1.0f);
	//glUniform3f(glGetUniformLocation(p, "lightColor"), 1.0f, 1.0f, 1.0f);
	//glUniform3f(glGetUniformLocation(p, "objectColor"), 1.0f,0.5f, 0.31f);
	//glUniform3f(glGetUniformLocation(p, "ambientColor"), 1.0f, 1.0f, 1.0f);
	

	//GLint iLocview;
	//iLocview = glGetUniformLocation(p, "um4v");
	//glUniformMatrix4fv(iLocview, 1, GL_FALSE, view_matrix.getTranspose());
	//GLint iLocrotate;
	//iLocrotate = glGetUniformLocation(p, "um4r");
	//glUniformMatrix4fv(iLocrotate, 1, GL_FALSE, R.getTranspose());
	//GLint iLocproj;
	//iLocproj = glGetUniformLocation(p, "um4p");
	//glUniformMatrix4fv(iLocproj, 1, GL_FALSE, project_matrix.getTranspose());

	Matrix4 mv;
	mv = view_matrix * T * R * S;
	glUniformMatrix4fv(iLocmv, 1, GL_FALSE, mv.getTranspose());
	glUniformMatrix4fv(iLocview, 1, GL_FALSE, view_matrix.getTranspose());
	glUniform3f(iLoccamera, main_camera.position.x,main_camera.position.y, main_camera.position.z);
	glUniform1f(iLocShininess,shininess);
	glUniform3f(iLocLd1, Ld1.x, Ld1.y, Ld1.z);
	glUniform3f(iLocLd2, Ld2.x, Ld2.y, Ld2.z);
	glUniform3f(iLocLd3, Ld3.x, Ld3.y, Ld3.z);
	glUniform3f(iLocLp1, Lp1.x, Lp1.y, Lp1.z);
	glUniform3f(iLocLp2, Lp2.x, Lp2.y, Lp2.z);
	glUniform3f(iLocLp3, Lp3.x, Lp3.y, Lp3.z);
	glUniform1i(iLocLightIdx, light_idx);
	glUniform1f(iLocspotcutoff,spotcutoff);

	//Matrix4 MV;
	//GLint iLocMV;
	//MV =  T * R * S;
	//iLocMV = glGetUniformLocation(p, "modelmat");
	//glUniformMatrix4fv(iLocMV,1,GL_FALSE, MV.getTranspose());
	//Matrix4 VP;
	//GLint iLocVP;
	//VP = view_matrix;
	//iLocVP = glGetUniformLocation(p, "viewPos");
	//glUniformMatrix4fv(iLocVP, 1, GL_FALSE, VP.getTranspose());
	
	glUniformMatrix4fv(iLocMVP, 1, GL_FALSE, mvp);
	for (int i = 0; i < models[cur_idx].shapes.size(); i++) 
	{   
		perpex = 0;
		glUniform1i(iLocperpex, perpex);
		glBindVertexArray(models[cur_idx].shapes[i].vao);
		glDrawArrays(GL_TRIANGLES, 0, models[cur_idx].shapes[i].vertex_count);
		glUniform3fv(iLocKa, 1, &(models[cur_idx].shapes[i].material.Ka[0]));
		glUniform3fv(iLocKd, 1, &(models[cur_idx].shapes[i].material.Kd[0]));
		glUniform3fv(iLocKs, 1, &(models[cur_idx].shapes[i].material.Ks[0]));
	}
	//const int WINDOW_WIDTH = 800;
	//const int WINDOW_HEIGHT = 600;
	glViewport(0, 0, WINDOW_WIDTH/2, WINDOW_HEIGHT);
	for (int i = 0; i < models[cur_idx].shapes.size(); i++)
	{	
		perpex = 1;
		glUniform1i(iLocperpex, perpex);
		glBindVertexArray(models[cur_idx].shapes[i].vao);
		glDrawArrays(GL_TRIANGLES, 0, models[cur_idx].shapes[i].vertex_count);
		glUniform3fv(iLocKa, 1, &(models[cur_idx].shapes[i].material.Ka[0]));
		glUniform3fv(iLocKd, 1, &(models[cur_idx].shapes[i].material.Kd[0]));
		glUniform3fv(iLocKs, 1, &(models[cur_idx].shapes[i].material.Ks[0]));
	}
	glViewport(WINDOW_WIDTH / 2, 0, WINDOW_WIDTH / 2, WINDOW_HEIGHT);
	
	
	
	//drawPlane();
}



void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	// [TODO] Call back function for keyboard

	if (action == GLFW_PRESS)
	{
		switch (key)
		{
		case GLFW_KEY_Z:
			cur_idx = (cur_idx + models.size() - 1) % models.size();
			return;
		case GLFW_KEY_X:
			cur_idx = (cur_idx + 1) % models.size();
			return;
		case GLFW_KEY_O:
			cur_proj_mode = Orthogonal;
			setOrthogonal();
			printf("o : go to orthogonal projection mode\n");
			break;
		case GLFW_KEY_P:
			cur_proj_mode = Perspective;
			setPerspective();
			printf("p : go to perspective projection mode\n");
			break;
		case GLFW_KEY_T:
			cur_trans_mode = GeoTranslation;
			break;
		case GLFW_KEY_S:
			cur_trans_mode = GeoScaling;
			break;
		case GLFW_KEY_R:
			cur_trans_mode = GeoRotation;
			break;
		case GLFW_KEY_E:
			cur_trans_mode = ViewEye;
			break;
		case GLFW_KEY_C:
			cur_trans_mode = ViewCenter;
			break;
		case GLFW_KEY_U:
			cur_trans_mode = ViewUp;
			break;
		case GLFW_KEY_J:
			cur_trans_mode = Shine;
			break;
		case GLFW_KEY_L:
			light_idx += 1;
			break;
		case GLFW_KEY_K:
			cur_trans_mode = Changelightmode;
		default:
			break;

		}
	}
	else
	{
		return;
	}
}

bool wheel_up = false;
float current_x, current_y;
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	// [TODO] scroll up positive, otherwise it would be negtive
	if (wheel_up == true)
	{
		current_x = xoffset;
		current_y = yoffset;
		float cxcy = current_x - current_y;
		switch (cur_trans_mode)
		{
		case GeoTranslation:
			models[cur_idx].position.z += 0.1 * (cxcy);
			break;
		case GeoScaling:
			models[cur_idx].scale.z += 0.5 * (cxcy);
			break;
		case GeoRotation:
			models[cur_idx].rotation.z += cxcy;
			break;
		case ViewEye:
			main_camera.position.z += 0.1 * (cxcy);
			setViewingMatrix();
			break;
		case ViewCenter:
			main_camera.center.z += 0.1 * (cxcy);
			setViewingMatrix();
			break;
		case ViewUp:
			main_camera.up_vector.z += 0.5 * (cxcy);
			setViewingMatrix();
			break;
		case Shine:
			shininess += 5*cxcy;
			break;
		case Changelightmode:
			if (light_idx % 3 == 0)
			{
				Ld1 = Ld1 + Vector3(1, 1, 1)*cxcy;
			}
			else if (light_idx % 3 == 1)
			{
				Ld2 = Ld2 + Vector3(1, 1, 1)*cxcy;
			}
			else if (light_idx % 3 == 2)
			{
				spotcutoff += -10*cxcy;
			}

		default:
			return;
			break;
		}
	}

	return;
}

bool mouse_press = false;
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	double xpos, ypos;
	// [TODO] mouse press callback function
	if (action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_LEFT)
	{
		printf("left_button_press\n");
		mouse_press = true;
		wheel_up = true;
		return;
	}
	if (action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_MIDDLE)
	{
		printf("wheel_up\n");
		mouse_press = false;
		wheel_up = true;
		return;
	}

	if (action == GLFW_RELEASE && button == GLFW_MOUSE_BUTTON_LEFT)
	{
		printf("left_button_release\n");
		mouse_press = false;
		wheel_up = true;
		return;
	}
}


float mouse_x, mouse_y;
static void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos)
{
	// [TODO] cursor position callback function
	float diff_x = xpos - mouse_x;
	float diff_y = mouse_y - ypos;
	mouse_x = xpos;
	mouse_y = ypos;


	if (mouse_press == true)
	{
		switch (cur_trans_mode)
		{
		case GeoTranslation:
			models[cur_idx].position.x += diff_x * 0.005;
			models[cur_idx].position.y += diff_y * 0.005;
			break;
		case GeoScaling:
			models[cur_idx].scale.x += diff_x * 0.005;
			models[cur_idx].scale.y += diff_y * 0.005;
			break;
		case GeoRotation:
			models[cur_idx].rotation.x +=  PI / 180.0 * diff_y * 10;
			models[cur_idx].rotation.y +=  PI / 180.0 * diff_x * 10;
			break;
		case ViewEye:
			main_camera.position.x += diff_x * 0.005;
			main_camera.position.y += diff_y * 0.005;
			setViewingMatrix();
			break;
		case ViewCenter:
			main_camera.center.x += diff_x * 0.005;
			main_camera.center.y += diff_y * 0.005;
			setViewingMatrix();
			break;
		case ViewUp:
			main_camera.up_vector.x += diff_x * 0.5;
			main_camera.up_vector.y += diff_y * 0.5;
			setViewingMatrix();
			break;
		case Changelightmode:
			if (light_idx % 3 == 0)
			{
				Lp1.x += diff_x * 0.1;
				Lp1.y += diff_y * 0.1;
			}
			else if (light_idx % 3 == 1)
			{
				Lp2.x += diff_x * 0.1;
				Lp2.y += diff_y * 0.1;
			}
			else if (light_idx % 3 == 2)
			{
				Lp3.x += diff_x * 0.01;
				Lp3.y += diff_y * 0.01;
			}
		default:
			return;
		}
	}
	else return;

}

void setShaders()
{
	GLuint v, f, p;
	char *vs = NULL;
	char *fs = NULL;
	v = glCreateShader(GL_VERTEX_SHADER);
	f = glCreateShader(GL_FRAGMENT_SHADER);
	vs = textFileRead("shader.vs");
	fs = textFileRead("shader.fs");
	glShaderSource(v, 1, (const GLchar**)&vs, NULL);
	glShaderSource(f, 1, (const GLchar**)&fs, NULL);
	free(vs);
	free(fs);
	GLint success;
	char infoLog[1000];
	// compile vertex shader
	glCompileShader(v);
	// check for shader compile errors
	glGetShaderiv(v, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(v, 1000, NULL, infoLog);
		std::cout << "ERROR: VERTEX SHADER COMPILATION FAILED\n" << infoLog << std::endl;
	}

	// compile fragment shader
	glCompileShader(f);
	// check for shader compile errors
	glGetShaderiv(f, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(f, 1000, NULL, infoLog);
		std::cout << "ERROR: FRAGMENT SHADER COMPILATION FAILED\n" << infoLog << std::endl;
	}

	// create program object
	p = glCreateProgram();

	// attach shaders to program object
	glAttachShader(p,f);
	glAttachShader(p,v);

	// link program
	glLinkProgram(p);
	// check for linking errors
	glGetProgramiv(p, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(p, 1000, NULL, infoLog);
		std::cout << "ERROR: SHADER PROGRAM LINKING FAILED\n" << infoLog << std::endl;
	}

	glDeleteShader(v);
	glDeleteShader(f);

	iLocMVP = glGetUniformLocation(p, "mvp");
	iLocKa = glGetUniformLocation(p, "Ka");
	iLocKd = glGetUniformLocation(p, "Kd");
	iLocKs = glGetUniformLocation(p, "Ks");
	iLocShininess = glGetUniformLocation(p, "shininess");
	iLocmv = glGetUniformLocation(p, "mv");
	iLocview = glGetUniformLocation(p, "view_matrix");
	iLoccamera = glGetUniformLocation(p, "cameraPos");
	iLocLightIdx = glGetUniformLocation(p, "lightIdx");
	iLocLd1 = glGetUniformLocation(p, "Ld1");
	iLocLd2 = glGetUniformLocation(p, "Ld2");
	iLocLd3 = glGetUniformLocation(p, "Ld3");
	iLocLp1 = glGetUniformLocation(p, "lightPos1");
	iLocLp2 = glGetUniformLocation(p, "lightPos2");
	iLocLp3 = glGetUniformLocation(p, "lightPos3");
	iLocLightIdx = glGetUniformLocation(p, "lightIdx");
	iLocspotcutoff = glGetUniformLocation(p, "spotCutoff");
	iLocperpex = glGetUniformLocation(p, "perpex");
	if (success)
		glUseProgram(p);
    else
    {
        system("pause");
        exit(123);
    }
}

void normalization(tinyobj::attrib_t* attrib, vector<GLfloat>& vertices, vector<GLfloat>& colors, vector<GLfloat>& normals, tinyobj::shape_t* shape)
{
	vector<float> xVector, yVector, zVector;
	float minX = 10000, maxX = -10000, minY = 10000, maxY = -10000, minZ = 10000, maxZ = -10000;

	// find out min and max value of X, Y and Z axis
	for (int i = 0; i < attrib->vertices.size(); i++)
	{
		//maxs = max(maxs, attrib->vertices.at(i));
		if (i % 3 == 0)
		{

			xVector.push_back(attrib->vertices.at(i));

			if (attrib->vertices.at(i) < minX)
			{
				minX = attrib->vertices.at(i);
			}

			if (attrib->vertices.at(i) > maxX)
			{
				maxX = attrib->vertices.at(i);
			}
		}
		else if (i % 3 == 1)
		{
			yVector.push_back(attrib->vertices.at(i));

			if (attrib->vertices.at(i) < minY)
			{
				minY = attrib->vertices.at(i);
			}

			if (attrib->vertices.at(i) > maxY)
			{
				maxY = attrib->vertices.at(i);
			}
		}
		else if (i % 3 == 2)
		{
			zVector.push_back(attrib->vertices.at(i));

			if (attrib->vertices.at(i) < minZ)
			{
				minZ = attrib->vertices.at(i);
			}

			if (attrib->vertices.at(i) > maxZ)
			{
				maxZ = attrib->vertices.at(i);
			}
		}
	}

	float offsetX = (maxX + minX) / 2;
	float offsetY = (maxY + minY) / 2;
	float offsetZ = (maxZ + minZ) / 2;

	for (int i = 0; i < attrib->vertices.size(); i++)
	{
		if (offsetX != 0 && i % 3 == 0)
		{
			attrib->vertices.at(i) = attrib->vertices.at(i) - offsetX;
		}
		else if (offsetY != 0 && i % 3 == 1)
		{
			attrib->vertices.at(i) = attrib->vertices.at(i) - offsetY;
		}
		else if (offsetZ != 0 && i % 3 == 2)
		{
			attrib->vertices.at(i) = attrib->vertices.at(i) - offsetZ;
		}
	}

	float greatestAxis = maxX - minX;
	float distanceOfYAxis = maxY - minY;
	float distanceOfZAxis = maxZ - minZ;

	if (distanceOfYAxis > greatestAxis)
	{
		greatestAxis = distanceOfYAxis;
	}

	if (distanceOfZAxis > greatestAxis)
	{
		greatestAxis = distanceOfZAxis;
	}

	float scale = greatestAxis / 2;

	for (int i = 0; i < attrib->vertices.size(); i++)
	{
		//std::cout << i << " = " << (double)(attrib.vertices.at(i) / greatestAxis) << std::endl;
		attrib->vertices.at(i) = attrib->vertices.at(i) / scale;
	}
	size_t index_offset = 0;
	for (size_t f = 0; f < shape->mesh.num_face_vertices.size(); f++) {
		int fv = shape->mesh.num_face_vertices[f];

		// Loop over vertices in the face.
		for (size_t v = 0; v < fv; v++) {
			// access to vertex
			tinyobj::index_t idx = shape->mesh.indices[index_offset + v];
			vertices.push_back(attrib->vertices[3 * idx.vertex_index + 0]);
			vertices.push_back(attrib->vertices[3 * idx.vertex_index + 1]);
			vertices.push_back(attrib->vertices[3 * idx.vertex_index + 2]);
			// Optional: vertex colors
			colors.push_back(attrib->colors[3 * idx.vertex_index + 0]);
			colors.push_back(attrib->colors[3 * idx.vertex_index + 1]);
			colors.push_back(attrib->colors[3 * idx.vertex_index + 2]);
			// Optional: vertex normals
			if (idx.normal_index >= 0) {
				normals.push_back(attrib->normals[3 * idx.normal_index + 0]);
				normals.push_back(attrib->normals[3 * idx.normal_index + 1]);
				normals.push_back(attrib->normals[3 * idx.normal_index + 2]);
			}
		}
		index_offset += fv;
	}
}

string GetBaseDir(const string& filepath) {
	if (filepath.find_last_of("/\\") != std::string::npos)
		return filepath.substr(0, filepath.find_last_of("/\\"));
	return "";
}

void LoadModels(string model_path)
{
	vector<tinyobj::shape_t> shapes;
	vector<tinyobj::material_t> materials;
	tinyobj::attrib_t attrib;
	vector<GLfloat> vertices;
	vector<GLfloat> colors;
	vector<GLfloat> normals;

	string err;
	string warn;

	string base_dir = GetBaseDir(model_path); // handle .mtl with relative path

#ifdef _WIN32
	base_dir += "\\";
#else
	base_dir += "/";
#endif

	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, model_path.c_str(), base_dir.c_str());

	if (!warn.empty()) {
		cout << warn << std::endl;
	}

	if (!err.empty()) {
		cerr << err << std::endl;
	}

	if (!ret) {
		exit(1);
	}

	printf("Load Models Success ! Shapes size %d Material size %d\n", shapes.size(), materials.size());
	model tmp_model;


	vector<PhongMaterial> allMaterial;
	for (int i = 0; i < materials.size(); i++)
	{
		PhongMaterial material;
		material.Ka = Vector3(materials[i].ambient[0], materials[i].ambient[1], materials[i].ambient[2]);
		material.Kd = Vector3(materials[i].diffuse[0], materials[i].diffuse[1], materials[i].diffuse[2]);
		material.Ks = Vector3(materials[i].specular[0], materials[i].specular[1], materials[i].specular[2]);
		allMaterial.push_back(material);
	}



	//GLuint iLocKa;
	//GLuint iLocKd;
	//GLuint iLocKs;
	//GLuint p;
	//glUniform3fv(iLocKa, 1, &(models[cur_idx].shapes[0].material.Ka[0]));
	//glUniform3fv(iLocKd, 1, &(models[cur_idx].shapes[0].material.Kd[0]));
	//glUniform3fv(iLocKs, 1, &(models[cur_idx].shapes[0].material.Ks[0]));
	
	//


	for (int i = 0; i < shapes.size(); i++)
	{

		vertices.clear();
		colors.clear();
		normals.clear();
		normalization(&attrib, vertices, colors, normals, &shapes[i]);
		// printf("Vertices size: %d", vertices.size() / 3);

		Shape tmp_shape;
		glGenVertexArrays(1, &tmp_shape.vao);
		glBindVertexArray(tmp_shape.vao);

		glGenBuffers(1, &tmp_shape.vbo);
		glBindBuffer(GL_ARRAY_BUFFER, tmp_shape.vbo);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GL_FLOAT), &vertices.at(0), GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		tmp_shape.vertex_count = vertices.size() / 3;

		glGenBuffers(1, &tmp_shape.p_color);
		glBindBuffer(GL_ARRAY_BUFFER, tmp_shape.p_color);
		glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(GL_FLOAT), &colors.at(0), GL_STATIC_DRAW);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glGenBuffers(1, &tmp_shape.p_normal);
		glBindBuffer(GL_ARRAY_BUFFER, tmp_shape.p_normal);
		
		glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(GL_FLOAT), &normals.at(0), GL_STATIC_DRAW);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);

		// not support per face material, use material of first face
		if (allMaterial.size() > 0)
			tmp_shape.material = allMaterial[shapes[i].mesh.material_ids[0]];
		tmp_model.shapes.push_back(tmp_shape);
	}
	shapes.clear();
	materials.clear();
	models.push_back(tmp_model);
}

void initParameter()
{
	proj.left = -1;
	proj.right = 1;
	proj.top = 1;
	proj.bottom = -1;
	proj.nearClip = 0.001;
	proj.farClip = 100.0;
	proj.fovy = 80;
	proj.aspect = (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT;

	main_camera.position = Vector3(0.0f, 0.0f, 2.0f);
	main_camera.center = Vector3(0.0f, 0.0f, 0.0f);
	main_camera.up_vector = Vector3(0.0f, 1.0f, 0.0f);

	setViewingMatrix();
	setPerspective();	//set default projection matrix as perspective matrix
}

void setupRC()
{
	// setup shaders
	setShaders();
	initParameter();

	// OpenGL States and Values


	glClearColor(0.2, 0.2, 0.2, 1.0);
	vector<string> model_list{ "../NormalModels/bunny5KN.obj", "../NormalModels/dragon10KN.obj", "../NormalModels/lucy25KN.obj", "../NormalModels/teapot4KN.obj", "../NormalModels/dolphinN.obj"};
	// [TODO] Load five model at here
	for (int i = 0; i < model_list.size(); i++)
		LoadModels(model_list[i]);
//	LoadModels(model_list[cur_idx]);
}

void glPrintContextInfo(bool printExtension)
{
	cout << "GL_VENDOR = " << (const char*)glGetString(GL_VENDOR) << endl;
	cout << "GL_RENDERER = " << (const char*)glGetString(GL_RENDERER) << endl;
	cout << "GL_VERSION = " << (const char*)glGetString(GL_VERSION) << endl;
	cout << "GL_SHADING_LANGUAGE_VERSION = " << (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
	if (printExtension)
	{
		GLint numExt;
		glGetIntegerv(GL_NUM_EXTENSIONS, &numExt);
		cout << "GL_EXTENSIONS =" << endl;
		for (GLint i = 0; i < numExt; i++)
		{
			cout << "\t" << (const char*)glGetStringi(GL_EXTENSIONS, i) << endl;
		}
	}
}


int main(int argc, char **argv)
{
    // initial glfw
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // fix compilation on OS X
#endif

    
    // create window
	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "108065466 HW2", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    
    
    // load OpenGL function pointer
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    
	// register glfw callback functions
    glfwSetKeyCallback(window, KeyCallback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetCursorPosCallback(window, cursor_pos_callback);

    glfwSetFramebufferSizeCallback(window, ChangeSize);
	glEnable(GL_DEPTH_TEST);
	// Setup render context
	setupRC();

	// main loop
    while (!glfwWindowShouldClose(window))
    {
        // render
        RenderScene();
        
        // swap buffer from back to front
        glfwSwapBuffers(window);
        
        // Poll input event
        glfwPollEvents();
    }
	
	// just for compatibiliy purposes
	return 0;
}
