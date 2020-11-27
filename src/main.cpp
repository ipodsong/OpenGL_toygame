#define TINYOBJLOADER_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "cgmath.h"			// slee's simple math library
#include "cgut.h"			// slee's OpenGL utility
#include "trackball.h"		// virtual trackball
#include <thread>
#include <iostream>
#include <cstdlib>
#include <mutex>
#include "tiny_obj_loader.h"
#include "stb_image.h"
#include "../include/irrKlang.h"
#include "ft2build.h"
#include <algorithm>
#include FT_FREETYPE_H
#pragma comment(lib, "freetype.lib")

FT_Library		freetype;
FT_Face			freetype_face;
GLuint			VAO, VBO;
GLuint			program_text;


using namespace irrklang;
ISoundEngine* engine;
ISoundEngine* t_engine;

#if defined(WIN32)
#include <conio.h>
#else
#include "common/conio.h"
#endif

#pragma comment(lib, "freetype.lib")
#pragma comment(lib, "../lib/Win32-visualStudio/irrKlang.lib")

#define num_obj 12
#define num_bg 1
#define main_c 0

//*******************************************************************
// global constants
static const char*	window_name = "cgbase - trackball";
static const char*	vert_shader_path = "../bin/shaders/trackball.vert";
static const char*	frag_shader_path = "../bin/shaders/trackball.frag";
static const char* mesh_vertex_path = "../bin/mesh/dragon.vertex.bin";
static const char* mesh_index_path = "../bin/mesh/dragon.index.bin";

//*******************************************************************
// window objects
GLFWwindow*	window = nullptr;
ivec2		window_size = ivec2(1024, 768 );	// initial window size

//*******************************************************************
// OpenGL objects
GLuint	program	= 0;	// ID holder for GPU program
GLuint	vertex_buffer[num_obj] = {};	// ID holder for vertex buffer
GLuint	index_buffer[num_obj] = {};	// ID holder for index buffer

GLuint	vertex_buffer2[num_bg] = {};	// ID holder for vertex buffer
GLuint	index_buffer2[num_bg] = {};	// ID holder for index buffer

//*******************************************************************
// global variables

int		frame			= 0;	// index of rendering frames
float	temp_angle		= 0;
float	tt1 = 0;
float	tt2 = 0;
bool	bUseIndexBuffer = true;
bool	PressQ			= false;
bool	PressW			= false;
bool	PressE			= false;
bool	PressR			= false;
bool	isSnap			= false;
std::string alert_msg;
std::string alert_msg2;
vec4 alert_color;
vec4 alert_color2;
vec4 alert_color3;
float comp_time = 0.f;
int node[10];
int c = 0;

//*******************************************************************
// scene objects
mesh*		pMesh = nullptr;
camera		cam;
trackball	tb;

Object t[num_obj];

background bg[num_bg];
//*******************************************************************
// holder of vertices and indices

// vertex
std::vector<vertex>	vertex_list;	// host-side vertices
std::vector<uint>	index_list;		// host-side indices


//*******************************************************************
//Prototype of functions
void Init_object();
void render_text(std::string text, GLint _x, GLint _y, GLfloat scale, vec4 color);
void text_init();




void check_drop() {
	if (t[0].pos.x > 900 || t[0].pos.x < -900 || t[0].pos.z > 900 || t[0].pos.z < -900) {
		t[0].des = t[0].pos - vec3(0.f, 500000.f, 0.f);
		t[0].speed = 1500.f;
		t[0].hit = true;
		alert_msg = "YOU DIE, Press F1 to restart";
		alert_color = vec4(1.f, 0.f, 0.f, 1.f);
	}
}

void shoot(int k, float x, float z) {

	if (t[k].state != 0) {
		t[k].look = vec3(x, 0.f, z * sqrt(2.f));
		t[t[k].state].look = normalize(vec3(x, 0.f, z * sqrt(2.f)));
		t[t[k].state].des = vec3( x + t[k].pos.x, 50.f, z + t[k].pos.z);
		t[t[k].state].angle = 3.f * PI / 2.f;
		t[t[k].state].flex = 1.f;
		t[t[k].state].state = 1;
		t[t[k].state].whoshoot = k;
		t[k].state = 0;
		//std::cout << "shooting!" << std::endl;
		if (k == 0)	alert_msg = "shooting!";
		ISoundEngine* t_engine = createIrrKlangDevice();
		t_engine->play2D("../media/Shooting.wav");
		
		//Sound :: 던질때 소리
	}
}

void grasp(int k) {
	for (int i = 2; i < num_obj; i++) {
		if (t[k].state == 0) {
			vec2 v = vec2(t[k].pos.x, t[k].pos.z) - vec2(t[i].pos.x, t[i].pos.z);
			if (length(v) < 60.f) {
				t[k].state = i;
				//std::cout << "Grasp!" << std::endl;
				if (k == 0)	alert_msg = "Grasp!";
				ISoundEngine* t_engine = createIrrKlangDevice();
				t_engine->play2D("../media/grab.wav");
				//Sound :: 주울때 소리
				break;
			}
		}
		else {
			//aleady grasped
		}
	}
}

void drop(int i) {
	//drop
	dvec2 pos; glfwGetCursorPos(window, &pos.x, &pos.y);
	float x, z;
	x = float(window_size.x - 1) / 2.f - float(pos.x);
	z = float(window_size.y - 1) / 2.f - float(pos.y);

	if (t[i].state != 0) {
		if (i == 0)	t[i].look = vec3(x, 0.f, z * sqrt(2.f));
		t[t[i].state].pos = vec3(t[i].pos.x, 50.f, t[i].pos.z);
		t[t[i].state].look = normalize(t[i].look);
		t[t[i].state].des = t[t[i].state].pos + vec3(0.f, -50.f, 0.f) + 40.f * t[t[i].state].look;
		t[t[i].state].speed = 600.f;
		t[t[i].state].angle = 3.f * PI / 2.f;
		t[t[i].state].flex = 1.f;
		//std::cout << "Drop!" << std::endl;
		if(i == 0)	alert_msg = "Drop!";

		ISoundEngine* t_engine = createIrrKlangDevice();
		t_engine->play2D("../media/Drop.wav");
		//Sound :: 떨어뜨릴때 소리
		t[i].state = 0;
	}
}

void snap(int i) {
	if (t[i].state % 2 == 1) {
		if (t[i].state == 3) {
			//success to snap!
			ISoundEngine* t_engine = createIrrKlangDevice();
			t_engine->play2D("../media/snap.mp3");

			if (i == 0)	alert_msg = "success to snap!";
			else alert_msg = "YOU DIE!";

			alert_color = vec4(1.f, 0.f, 0.f, 1.f);

			alert_msg2 = "Press F1 to restart!";
			alert_color3 = vec4(1.0f, 1.0f, 1.0f, 1.0f);
			isSnap = true;
			glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		}
		else {
			//fake guantlet!
			vec3 lo = normalize(t[i].look);

			drop(i);
			if (i == 0)	alert_msg = "fake guantlet!";

			t[i].hit = true;
			t[i].hit_time = float(glfwGetTime());
		}

		t[i].state = 0;
	}
	else {
		//It is not gauntlet!
		if (i == 0)	alert_msg = "It is not gauntlet!";
	}
}

void move_computer() {
	float t1 = float(glfwGetTime());

	if ((t1 - comp_time) > 1.5f && c < 10) {
		comp_time = t1;
		
		t[1].look = normalize(vec3(t[node[c]].pos.x, 0.f, t[node[c]].pos.z));
		t[1].des = vec3(t[node[c]].pos.x, 0.f, t[node[c]].pos.z);
		t[1].flex = 1.f;
		if (t[0].state == node[c])	c++;

		if ((t[node[c]].pos - t[1].pos).length() < 60.f) {
			grasp(1);
			if (node[c] % 2 == 0)	shoot(1, -t[1].pos.x + t[0].pos.x, -t[1].pos.z + t[0].pos.z);
			else	snap(1);
			c++;
		}
	}
}

void loadModel(std::string n_obj, std::string n_mtl) {
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;
	vertex v;
	vertex_list.clear();
	index_list.clear();

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, n_obj.c_str())) {
		throw std::runtime_error(warn + err);
	}

	for (const auto& shape : shapes) {
		for (const auto& index : shape.mesh.indices) {
			v.pos = { 0.f,0.f,0.f };
			v.norm = { 0.f,0.f,0.f };
			v.tex = { 0.f,0.f };
			if (attrib.vertices.size() >= (size_t)(3 * index.vertex_index + 2)) {
				v.pos = {
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2]
				};
			}
			if (attrib.normals.size() >= (size_t)(3 * index.vertex_index + 2)) {
				v.norm = {
					attrib.normals[3 * index.vertex_index + 0],
					attrib.normals[3 * index.vertex_index + 1],
					attrib.normals[3 * index.vertex_index + 2]
				};
			}
			if (attrib.texcoords.size() >= (size_t)(2 * index.texcoord_index + 1)) {
				v.tex = {
					attrib.texcoords[2 * index.texcoord_index + 0],
					1.f - attrib.texcoords[2 * index.texcoord_index + 1]
				};
			}

			vertex_list.push_back(v);
			index_list.push_back(index_list.size());
		}
	}

}



void set_cam() {
	vec3 c = cam.eye - t[main_c].pos;
	vec3 b = cam.eye - cam.at;
	cam.eye -= c - b;
	cam.at -= c - b;
	cam.view_matrix = mat4::look_at(cam.eye, cam.at, cam.up);

}

void moving_obj() {
	tt2 = tt1;
	tt1 = float(glfwGetTime());
	vec3 v;
	
	for (int i = 0; i < num_obj; i++) {
		v = t[i].des - t[i].pos;
		if (length(v) > 10.f && t[i].hit == false) {
			t[i].pos += t[i].speed * (tt1 - tt2) * normalize(v);
		}
		else {
			t[i].flex = 0.f;
			if (i != 0 && i != 1) {
				t[i].state = 0;
			}
		}
	}

	for (int i = 2; i < num_obj; i++) {
		if (t[0].state == i) {
			t[i].pos = t[0].pos + vec3(0.f, 200.f, 0.f);
			t[i].des = t[0].pos + vec3(0.f, 200.f, 0.f);
		}
	}
}

void check_hit() {
	vec2 v;
	for (int i = 2; i < num_obj; i++) {
		if (t[i].state == 1) {
			for (int k = 0; k < 2; k++) {
				v = vec2(t[i].pos.x, t[i].pos.z) - vec2(t[k].pos.x, t[k].pos.z);

				if (v.length() < 50.f && t[i].whoshoot != k) {
					t[k].state = 0;
					t[k].hit = true;
					t[i].state = 0;
					t[i].des = t[k].pos + vec3(0.f, 50.f, 0.f) + 50.f * normalize(t[i].pos - t[i].des);

					t[k].hit_time = float(glfwGetTime());
					//std::cout << "hit" << std::endl;
					ISoundEngine* t_engine = createIrrKlangDevice();
					t_engine->play2D("../media/Hit_sound.wav");
					//Sound :: 맞을때 소리
				}
			}
		}

	}
}

void recover() {
	for (int i = 0; i < 2; i++) {
		if (t[i].hit == true) {
			if (t[i].hit_time < float(glfwGetTime()) - 1.5f)	t[i].hit = false;
		}
	}
}

mat4 getModelMat(int i) {
	mat4 model_matrix;
	//rotation
	vec2 y_look = vec2(t[i].look.x, t[i].look.z);
	vec2 y_basis_rotate = vec2(0.f, 1.f);

	vec4 body_rotate_basis = mat4::rotate(vec3(0.f, 1.f, 0.f), PI / 2.f) * vec4(t[i].look.x, 0.f, t[i].look.z, 1.f);

	if (t[i].look.x > 0.f) {
		temp_angle = acos(dot(y_look, y_basis_rotate) / (abs(length(y_look)) * abs(length(y_basis_rotate))));

		if ((abs(t[i].angle2 - temp_angle) > PI / 18.f)) {

			if (t[i].angle2 < temp_angle) {
				t[i].angle2 += PI / 60.f;
			}
			else {
				if (t[i].angle2 - temp_angle > PI) {
					t[i].angle2 += PI / 60.f;
					if (t[i].angle2 > 2.f * PI)	t[i].angle2 = 0.001f;
				}
				else {
					t[i].angle2 -= PI / 60.f;
				}
			}
		}

		if (t[i].flex == 1 && t[i].angle < t[i].full_angle) {
			t[i].angle += PI / 90.f;
		}
		else if (t[i].flex == 0 && t[i].angle > 0.f) {
			t[i].angle -= PI / 90.f;
		}


		model_matrix =
			mat4::translate(t[i].pos)
			* mat4::rotate(normalize(vec3(body_rotate_basis.x, body_rotate_basis.y, body_rotate_basis.z)), t[i].angle)
			* mat4::rotate(normalize(vec3(0.f, 1.f, 0.f)), t[i].angle2);

	}
	else {
		temp_angle = 2.f * PI - acos(dot(y_look, y_basis_rotate) / (abs(length(y_look)) * abs(length(y_basis_rotate))));

		if ((abs(t[i].angle2 - temp_angle) > PI / 18.f)) {

			if (t[i].angle2 > temp_angle) {
				t[i].angle2 -= PI / 20.f;
			}
			else {
				if (temp_angle - t[i].angle2 > PI) {
					t[i].angle2 -= PI / 20.f;
					if (t[i].angle2 < 0.f)	t[i].angle2 = 2.f * PI;
				}
				else {
					t[i].angle2 += PI / 20.f;
				}
			}
		}

		if (t[i].flex == 1 && t[i].angle < t[i].full_angle) {
			t[i].angle += PI / 30.f;
		}
		else if (t[i].flex == 0 && t[i].angle > 0.f) {
			t[i].angle -= PI / 30.f;
		}


		model_matrix =
			mat4::translate(t[i].pos)
			* mat4::rotate(normalize(vec3(body_rotate_basis.x, body_rotate_basis.y, body_rotate_basis.z)), t[i].angle)
			* mat4::rotate(normalize(vec3(0.f, 1.f, 0.f)), t[i].angle2);

	}

	if (t[i].hit == true) {
		model_matrix =
			mat4::translate(t[i].pos)
			* mat4::rotate(normalize(vec3(body_rotate_basis.x, body_rotate_basis.y, body_rotate_basis.z)), PI / 2.f)
			* mat4::rotate(normalize(vec3(0.f, 1.f, 0.f)), t[i].angle2);
	}

	return model_matrix;
}

void update()
{
	// update projection matrix
	cam.aspect_ratio = window_size.x / float(window_size.y);
	cam.projection_matrix = mat4::perspective(cam.fovy, cam.aspect_ratio, cam.dnear, cam.dfar);


	moving_obj();
	check_hit();
	recover();
	check_drop();
	set_cam();
	move_computer();

	// update uniform variables in vertex/fragment shaders
	GLint uloc;
	uloc = glGetUniformLocation(program, "view_matrix");			if (uloc > -1) glUniformMatrix4fv(uloc, 1, GL_TRUE, cam.view_matrix);
	uloc = glGetUniformLocation(program, "projection_matrix");	if (uloc > -1) glUniformMatrix4fv(uloc, 1, GL_TRUE, cam.projection_matrix);
	uloc = glGetUniformLocation(program, "aspect_ratio");		if (uloc > -1) glUniform1f(uloc, window_size.x / float(window_size.y));
	uloc = glGetUniformLocation(program, "snap");		if (uloc > -1) glUniform1i(uloc, isSnap);

}

void render(){
	// clear screen (with background color) and clear depth buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// notify GL that we use our own program and buffers
	glUseProgram(program);
	
	for (size_t i = 0; i < num_obj; i++) {
		if (vertex_buffer[i])	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer[i]);
		if (index_buffer[i])	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer[i]);

		GLint uloc;
		mat4 model_matrix;

		if (t[i].texture != NULL) {
			glActiveTexture(GL_TEXTURE0);						// select the texture slot to bind
			glBindTexture(GL_TEXTURE_2D, t[i].texture);
			glUniform1i(glGetUniformLocation(program, "TEX"), 0);	 // GL_TEXTURE0
		}

		model_matrix = getModelMat(i);

		uloc = glGetUniformLocation(program, "model_matrix");			if (uloc > -1) glUniformMatrix4fv(uloc, 1, GL_TRUE, model_matrix);
		uloc = glGetUniformLocation(program, "scale");			if (uloc > -1) glUniform1f(uloc, t[i].scale);

		// bind vertex attributes to your shader program
		const char* vertex_attrib[] = { "position", "normal", "texcoord" };
		size_t		attrib_size[] = { sizeof(vertex().pos), sizeof(vertex().norm), sizeof(vertex().tex) };
		for (size_t k = 0, kn = std::extent<decltype(vertex_attrib)>::value, byte_offset = 0; k < kn; k++, byte_offset += attrib_size[k - 1])
		{
			GLuint loc = glGetAttribLocation(program, vertex_attrib[k]); if (loc >= kn) continue;
			glEnableVertexAttribArray(loc);
			glVertexAttribPointer(loc, attrib_size[k] / sizeof(GLfloat), GL_FLOAT, GL_FALSE, sizeof(vertex), (GLvoid*)byte_offset);
		}
		// render vertices: trigger shader programs to process vertex data

		if (bUseIndexBuffer)	glDrawElements(GL_TRIANGLES, t[i].index_list.size(), GL_UNSIGNED_INT, nullptr);

	}

	for (size_t i = 0; i < num_bg; i++) {
		if (vertex_buffer2[i])	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer2[i]);
		if (index_buffer2[i])	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer2[i]);

		GLint uloc;
		mat4 model_matrix;

		if (bg[i].texture != NULL) {
			glActiveTexture(GL_TEXTURE0);						// select the texture slot to bind
			glBindTexture(GL_TEXTURE_2D, bg[i].texture);
			glUniform1i(glGetUniformLocation(program, "TEX"), 0);	 // GL_TEXTURE0
		}

		model_matrix = mat4::translate(bg[i].pos);

		uloc = glGetUniformLocation(program, "model_matrix");			if (uloc > -1) glUniformMatrix4fv(uloc, 1, GL_TRUE, model_matrix);
		uloc = glGetUniformLocation(program, "scale");			if (uloc > -1) glUniform1f(uloc, bg[i].scale);

		// bind vertex attributes to your shader program
		const char* vertex_attrib[] = { "position", "normal", "texcoord" };
		size_t		attrib_size[] = { sizeof(vertex().pos), sizeof(vertex().norm), sizeof(vertex().tex) };
		for (size_t k = 0, kn = std::extent<decltype(vertex_attrib)>::value, byte_offset = 0; k < kn; k++, byte_offset += attrib_size[k - 1])
		{
			GLuint loc = glGetAttribLocation(program, vertex_attrib[k]); if (loc >= kn) continue;
			glEnableVertexAttribArray(loc);
			glVertexAttribPointer(loc, attrib_size[k] / sizeof(GLfloat), GL_FLOAT, GL_FALSE, sizeof(vertex), (GLvoid*)byte_offset);
		}
		// render vertices: trigger shader programs to process vertex data

		if (bUseIndexBuffer)	glDrawElements(GL_TRIANGLES, bg[i].index_list.size(), GL_UNSIGNED_INT, nullptr);

	}

	//glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	//glDisable(GL_TEXTURE_2D);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glUseProgram(program_text);
	render_text(alert_msg, (GLint)(window_size.x / 2.f - alert_msg.length() * 10.f), 100, 0.8f, alert_color);
	render_text(alert_msg2, (GLint)(window_size.x / 2.f - alert_msg2.length() * 10.f), 200, 0.8f, alert_color2);
	render_text("Player", (GLint)(window_size.x / 2.f - window_size.x / 40.f), (GLint)(window_size.y / 2.f - window_size.y / 8.f), 0.4f, alert_color3);
	glDisable(GL_BLEND);
	//glDisable(GL_CULL_FACE);
	//glEnable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);
	// swap front and back buffers, and display to screen
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// notify GL that we use our own program and buffers
	glUseProgram(program);



	glfwSwapBuffers( window );
	
}

void reshape( GLFWwindow* window, int width, int height )
{
	// set current viewport in pixels (win_x, win_y, win_width, win_height)
	// viewport: the window area that are affected by rendering 
	window_size = ivec2(width,height);
	glViewport( 0, 0, width, height );
}

void print_help()
{
	printf(" [help]\n" );

	printf("\n Find the REAL gauntlet and snaps it faster than the computer !!!! \n\n");
	printf(" 'Q'  :  Throw the object in the direction of the mouse cursor while grasped it.\n");
	printf(" 'W'  :  Take the object from the ground. \n");
	printf(" 'E'  :  Dropping object to the ground. \n");
	printf(" 'R'  :  When holding a gauntlet, snaps it. \n");
	

	printf( "\n" );
}



void keyboard(GLFWwindow* window, int key, int scancode, int action, int mods)
{

	if (action == GLFW_PRESS) {
		if (key == GLFW_KEY_F1)	Init_object();
		if (key == GLFW_KEY_Q) {
			//shooting
			dvec2 pos; glfwGetCursorPos(window, &pos.x, &pos.y);
			float x, z;
			x = float(window_size.x - 1) / 2.f - float(pos.x);
			z = float(window_size.y - 1) / 2.f - float(pos.y);
			shoot(0, (700.f / (window_size.x / 2.f)) * x, (window_size.y / 2.f / 300.f) * z * sqrt(2.f));
		}
		if (key == GLFW_KEY_W) {
			//grasp
			grasp(0);
		}
		if (key == GLFW_KEY_E) {
			//drop
			drop(0);
		}
		if (key == GLFW_KEY_R) {
			//snap
			snap(0);
		}

	


	}
}

void mouse(GLFWwindow * window, int button, int action, int mods){
	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {

		ISoundEngine* t_engine = createIrrKlangDevice();
		t_engine->play2D("../media/fly.wav");

		dvec2 pos; glfwGetCursorPos(window, &pos.x, &pos.y);

		float x, z;

		x = float(window_size.x - 1) / 2.f - float(pos.x);
		z = float(window_size.y - 1) / 2.f - float(pos.y);

		if (t[0].hit == false) {
			t[main_c].look = normalize(vec3(x, 0.f, z * sqrt(2.f)));
			t[main_c].des = vec3((700.f/ (window_size.x / 2.f)) * x + t[main_c].pos.x, 0.f, (window_size.y / 2.f / 300.f) * z * sqrt(2.f) + t[main_c].pos.z);
			t[main_c].flex = 1.f;
		}

		//std::cout << "x : " << t[main_c].look.x << " y : " << t[main_c].look.z << std::endl;
	}
}

void motion(GLFWwindow * window, double x, double y)
{
	
}

void set_Object(int obj_num, vec3 position, vec3 lookat, vec3 destination, float speed, float scale, std::string objfile_name, std::string mtlfile_name) {
	t[obj_num].pos = position;
	t[obj_num].look = lookat;
	t[obj_num].des = destination;
	t[obj_num].speed = speed;
	t[obj_num].scale = scale;

	//Make vertex list and index list
	loadModel(objfile_name.c_str(), mtlfile_name.c_str());
	t[obj_num].vertex_list = vertex_list;
	t[obj_num].index_list = index_list;
}

void set_Background(int bg_num, vec3 position, vec3 lookat, float scale, std::string objfile_name, std::string mtlfile_name) {
	bg[bg_num].pos = position;
	bg[bg_num].look = lookat;
	bg[bg_num].scale = scale;

	//Make vertex list and index list
	loadModel(objfile_name.c_str(), mtlfile_name.c_str());
	bg[bg_num].vertex_list = vertex_list;
	bg[bg_num].index_list = index_list;
}

bool load_texture(const char* path, GLuint* texture) {

	int width, height, comp = 3;
	unsigned char* pimage0 = stbi_load(path, &width, &height, &comp, 3);
	if (comp == 1) comp = 3;
	int stride0 = width * comp, stride1 = (stride0 + 3) & (~3);	// 4-byte aligned stride
	unsigned char* pimage = (unsigned char*)malloc(sizeof(unsigned char) * stride1 * height);
	for (int y = 0; y < height; y++) memcpy(pimage + (height - 1 - y) * stride1, pimage0 + y * stride0, stride0); // vertical flip

	glGenTextures(1, texture);

	glBindTexture(GL_TEXTURE_2D, *texture);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8 /* GL_RGB for legacy GL */, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, pimage);

	// allocate and create mipmap
	int mip_levels = miplevels(window_size.x, window_size.y);
	for (int k = 1, w = width >> 1, h = height >> 1; k < mip_levels; k++, w = max(1, w >> 1), h = max(1, h >> 1))
		glTexImage2D(GL_TEXTURE_2D, k, GL_RGB8 /* GL_RGB for legacy GL */, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
	glGenerateMipmap(GL_TEXTURE_2D);

	// configure texture parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	// release the new image
	free(pimage);

	return true;
}


void Init_object() {
	std::cout << "init object start..." << std::endl;
	vec3 pos1;
	//****************************************************************************************************************************************
	//Object

	//IronMan(main)
	set_Object(0, vec3(0.f, 0.f, -200.f), vec3(0.f, 0.f, 1.f), vec3(0.f, 0.f, -200.f), 300.f, 80.f, "../bin/mesh/Mark_42.obj", "");
	load_texture("../bin/textures/42.jpg", &t[0].texture);
	t[0].full_angle = PI / 4.f;

	//thanos
	set_Object(1, vec3(0.f, 0.f, 200.f), vec3(0.f, 0.f, -1.f), vec3(0.f, 0.f, 200.f), 300.f, 80.f, "../bin/mesh/Mark_42.obj", "");
	load_texture("../bin/textures/42.jpg", &t[1].texture);
	t[1].full_angle = PI / 4.f;

	srand((unsigned int)time(NULL));
	for (int i = 2; i < 12; i += 2) {
		pos1 = vec3((float)(rand() % 1700 - 850), 50.f, (float)(rand() % 1700 - 850));
		//Mjolnir
		set_Object(i, pos1, vec3(0.f, 0.f, 1.f), pos1, 1500.f, 4.f, "../bin/mesh/Hammer.obj", "");
		load_texture("../bin/textures/thor_color.jpg", &t[i].texture);
		t[i].angle = 3.f * PI / 2.f;

		pos1 = vec3((float)(rand() % 1700 - 850), 50.f, (float)(rand() % 1700 - 850));
		//glove
		set_Object(i + 1, pos1, vec3(0.f, 0.f, -1.f), pos1, 600.f, 3.f, "../bin/mesh/glove.obj", "");
		load_texture("../bin/textures/glove.jpg", &t[i+1].texture);

	}
	std::cout << "init object complete!" << std::endl;

	//****************************************************************************************************************************************
	//Background
	set_Background(0, vec3(0.f, 0.f, 0.f), vec3(0.f, 0.f, 1.f), 300.f, "../bin/mesh/Floor.obj", "");
	load_texture("../bin/textures/Floor.jpg", &bg[0].texture);

	//Sound :: 배경음악
	ISoundEngine* engine = createIrrKlangDevice();
	engine->play2D("../media/Avengers_Theme.mp3", true);

	alert_msg = "Find guantlet and snap it!";
	alert_color = vec4(0.5f, 0.8f, 0.2f, 1.0f);
	alert_msg2 = "";
	alert_color2 = vec4(0.f, 0.f, 0.f, 1.0f);
	alert_color3 = vec4(0.5f, 0.8f, 0.2f, 1.0f);

	isSnap = false;
	glClearColor(39 / 255.0f, 40 / 255.0f, 34 / 255.0f, 1.0f);
	t[0].state = 0;
	t[1].state = 0;


	for (int i = 2; i < 12; i++) {
		node[i - 2] = i;
		std::random_shuffle(node, node + 10);
	}

	c = 0;
}


bool user_init(){
	// log hotkeys
	print_help();
	
	// init GL states
	glClearColor(39 / 255.0f, 40 / 255.0f, 34 / 255.0f, 1.0f);	// set clear color
						// turn on backface culling
	glEnable(GL_DEPTH_TEST);								// turn on depth tests
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glActiveTexture(GL_TEXTURE0);

	Init_object();

	text_init();

	return true;
}

void user_finalize(){
	engine->drop();
	t_engine->drop();
}

void set_buffer() {
	for (int i = 0; i < num_obj; i++) {
		// clear and create new buffers
		if (vertex_buffer[i])	glDeleteBuffers(1, &vertex_buffer[i]);	vertex_buffer[i] = 0;
		if (index_buffer[i])	glDeleteBuffers(1, &index_buffer[i]);	index_buffer[i] = 0;

		// generation of vertex buffer: use vertex_list as it is
		glGenBuffers(1, &vertex_buffer[i]);
		glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer[i]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertex) * t[i].vertex_list.size(), &t[i].vertex_list[0], GL_STATIC_DRAW);

		// geneation of index buffer
		glGenBuffers(1, &index_buffer[i]);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer[i]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint) * t[i].index_list.size(), &t[i].index_list[0], GL_STATIC_DRAW);

	}
	//std::cout << "set buffer 1 complete" << std::endl;
}

void set_buffer2() {
	for (int i = 0; i < num_bg; i++) {
		// clear and create new buffers
		if (vertex_buffer2[i])	glDeleteBuffers(1, &vertex_buffer2[i]);	vertex_buffer2[i] = 0;
		if (index_buffer2[i])	glDeleteBuffers(1, &index_buffer2[i]);	index_buffer2[i] = 0;

		// generation of vertex buffer: use vertex_list as it is
		glGenBuffers(1, &vertex_buffer2[i]);
		glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer2[i]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertex) * bg[i].vertex_list.size(), &bg[i].vertex_list[0], GL_STATIC_DRAW);

		// geneation of index buffer
		glGenBuffers(1, &index_buffer2[i]);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer2[i]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint) * bg[i].index_list.size(), &bg[i].index_list[0], GL_STATIC_DRAW);

	}
	std::cout << "Buffering complete" << std::endl;
}

int main( int argc, char* argv[] )
{
	// initialization
	if(!glfwInit()){ printf( "[error] failed in glfwInit()\n" ); return 1; }

	// create window and initialize OpenGL extensions
	if(!(window = cg_create_window( window_name, window_size.x, window_size.y ))){ glfwTerminate(); return 1; }
	if(!cg_init_extensions( window )){ glfwTerminate(); return 1; }	// version and extensions

	// initializations and validations
	if(!(program=cg_create_program( vert_shader_path, frag_shader_path ))){ glfwTerminate(); return 1; }	// create and compile shaders/program
	if(!user_init()){ printf( "Failed to user_init()\n" ); glfwTerminate(); return 1; }					// user initialization

	// register event callbacks
	glfwSetWindowSizeCallback( window, reshape );	// callback for window resizing events
    glfwSetKeyCallback( window, keyboard );			// callback for keyboard events
	glfwSetMouseButtonCallback( window, mouse );	// callback for mouse click inputs
	glfwSetCursorPosCallback( window, motion );		// callback for mouse movement

	set_buffer();
	set_buffer2();

	// enters rendering/event loop
	for( frame=0; !glfwWindowShouldClose(window); frame++ )
	{
		glfwPollEvents();	// polling and processing of events
		update();			// per-frame update
		render();			// per-frame render
	}

	// normal termination
	user_finalize();
	cg_destroy_window(window);

	return 0;
}





static const char*	font_path = "C:/Windows/Fonts/consola.ttf";
static const char*	vert_text_path = "../bin/shaders/text.vert";
static const char*	frag_text_path = "../bin/shaders/text.frag";

struct ft_char_t
{
	GLuint	textureID;
	ivec2	size;
	ivec2	bearing;
	GLuint	advance;
};
std::map<GLchar, ft_char_t> ft_char_list;

void text_init() {
	FT_Init_FreeType(&freetype);
	FT_New_Face(freetype, font_path, 0, &freetype_face);
	FT_Set_Pixel_Sizes(freetype_face, 0, 48);
	
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	for (GLubyte c = 0; c < 128; c++)
	{
		// Load character glyph
		FT_Load_Char(freetype_face, c, FT_LOAD_RENDER);
		// Generate texture
		GLuint texture_text;
		glGenTextures(1, &texture_text);
		glBindTexture(GL_TEXTURE_2D, texture_text);
		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			GL_RED,
			freetype_face->glyph->bitmap.width,
			freetype_face->glyph->bitmap.rows,
			0,
			GL_RED,
			GL_UNSIGNED_BYTE,
			freetype_face->glyph->bitmap.buffer
		);
		// Set texture options
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// Now store character for later use
		ft_char_t character =
		{
			texture_text,
			ivec2(freetype_face->glyph->bitmap.width, freetype_face->glyph->bitmap.rows),
			ivec2(freetype_face->glyph->bitmap_left, freetype_face->glyph->bitmap_top),
			(GLuint)freetype_face->glyph->advance.x
		};
		ft_char_list.insert(std::pair<GLchar, ft_char_t>(c, character));
	} // loop end

	FT_Done_Face(freetype_face);
	FT_Done_FreeType(freetype);

	program_text = cg_create_program(vert_text_path, frag_text_path); // create text shader program
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	// Vertices below contain x, y, texcoord x, texcoord y
	GLfloat vertices[6][4] = {
		{ 0, 1, 0.0, 0.0 },
		{ 0, 0, 0.0, 1.0 },
		{ 1, 0, 1.0, 1.0 },
		{ 0, 1, 0.0, 0.0 },
		{ 1, 0, 1.0, 1.0 },
		{ 1, 1, 1.0, 0.0 },
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, vertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}


void render_text(std::string text, GLint _x, GLint _y, GLfloat scale, vec4 color)
{
	// Activate corresponding render state
	extern ivec2 window_size;
	GLfloat x = GLfloat(_x);
	GLfloat y = GLfloat(_y);
	glUseProgram(program_text);
	glUniform4f(glGetUniformLocation(program_text, "textColor"), color.r, color.g, color.b, color.a);
	glActiveTexture(GL_TEXTURE0);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	mat4 text_offset = mat4(1 / (window_size.x / 2.0f), 0.0f, 0.0f, -1.0f,
		0.0f, 1 / (window_size.y / 2.0f), 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f);

	std::string::const_iterator c;
	for (c = text.begin(); c != text.end(); c++)
	{
		ft_char_t ch = ft_char_list[*c];
		mat4 text_size = mat4(scale * float(ch.size.x), 0.0f, 0.0f, 0.0f,
			0.0f, scale * float(ch.size.y), 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		);
		mat4 text_translate = mat4(1.0f, 0.0f, 0.0f, x + scale * float(ch.bearing.x),
			0.0f, 1.0f, 0.0f, -y + scale * float(-(ch.size.y - ch.bearing.y)),
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		);
		mat4 text_matrix = mat4();
		text_matrix = text_translate * text_size * text_matrix;
		text_matrix = text_offset * text_matrix;
		glUniformMatrix4fv(glGetUniformLocation(program_text, "text_matrix"), 1, GL_TRUE, text_matrix);
		// Render glyph texture over quad
		glBindTexture(GL_TEXTURE_2D, ch.textureID);
		// Render quad
		glDrawArrays(GL_TRIANGLES, 0, 6);
		// Now advance cursors for next glyph (note that advance is number of 1/64 pixels)
		x += (ch.advance >> 6) * scale; // Bitshift by 6 to get value in pixels (2^6 = 64)
	} // iteration end
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);

}