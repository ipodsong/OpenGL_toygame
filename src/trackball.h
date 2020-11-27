#ifndef __TRACKBALL_H__
#define __TRACKBALL_H__
#include "cgmath.h"

struct camera
{
	vec3	eye = vec3(0, 900, -900);
	vec3	at = vec3(0, 0, 0);
	vec3	up = normalize(vec3(0, 1, 1));
	mat4	view_matrix = mat4::look_at(eye, at, up);

	float	fovy = PI / 4.0f; // must be in radian
	float	aspect_ratio = 1.0f;
	float	dnear = 1.0f;
	float	dfar = 10000.0f;
	mat4	projection_matrix;
};

struct background {
	vec3 pos;
	vec3 look;
	float scale = 1,f;
	GLuint texture = NULL;

	// vertex
	std::vector<vertex>	vertex_list;	// host-side vertices
	std::vector<uint>	index_list;		// host-side indices
};

struct Object {
	vec3 pos;			//Position of object
	vec3 look;		//Direction of object, it must be normalized!
	vec3 des;
	bool moving = false;
	float speed = 0.f;		//Velocity of object
	float scale = 1.f;
	float angle = 0.f;
	float full_angle = 3.f * PI / 2.f;
	float flex = 0.f;
	float angle2 = 0.f;
	bool hit = false;
	float hit_time;
	int whoshoot = -1;

	int state = 0;
	// 0 - normal
	// 1 - moving
	// 2 - grasping hammer
	// 3 - grasping glove

	GLuint texture = NULL;

	// vertex
	std::vector<vertex>	vertex_list;	// host-side vertices
	std::vector<uint>	index_list;		// host-side indices
};

struct Skill {
	float Cool_time;	//Cooltime of skill
	float Damage;
	int N_skill;		//Count of skill object
	vec3 Direction;		//Direction of skill, it must be normalized!
	float scale;		//Scale of skill object
};

struct trackball
{
	bool	b_tracking = false;
	float	scale;			// controls how much rotation is applied
	mat4	view_matrix0;	// initial view matrix
	vec2	m0;				// the last mouse position

	vec3 temp_eye;
	vec3 temp_at;
	vec3 temp_up;


	trackball( float rot_scale=1.0f ):scale(rot_scale){}
	bool is_tracking() const { return b_tracking; }
	void begin( const mat4& view_matrix, float x, float y, camera* c )
	{
		b_tracking = true;			// enable trackball tracking
		m0 = vec2(x,y)*2.0f-1.0f;	// convert (x,y) in [0,1] to [-1,1]
		view_matrix0 = view_matrix;	// save current view matrix
		temp_eye = c->eye;
		temp_at = c->at;
		temp_up = c->up;
	}
	void end() { b_tracking = false; }

	mat4 update( float x, float y, camera* c )
	{
		// retrive the current mouse position
		vec2 m = vec2(x,y)*2.0f - 1.0f; // normalize xy

		// project a 2D mouse position to a unit sphere
		static const vec3 p0 = vec3(0,0,1.0f);	// reference position on sphere
		vec3 p1 = vec3(m.x-m0.x, m0.y-m.y,0);	// displacement with vertical swap
		if(!b_tracking||length(p1)<0.0001f) return view_matrix0;			// ignore subtle movement

		p1 *= scale;														// apply rotation scale
		p1 = vec3(p1.x,p1.y,sqrtf(max(0,1.0f-length2(p1)))).normalize();	// back-project z=0 onto the unit sphere

		// find rotation axis and angle
		// - right-mult of mat3 = inverse view rotation to world
		// - i.e., rotation is done in the world coordinates
		vec3 n = p0.cross(p1)*mat3(view_matrix0);
		float angle = asin( min(n.length(),0.999f) );

		// return resulting rotation matrix
		//return view_matrix0 * mat4::rotate(n.normalize(),angle);


		vec4 update_eye = (mat4::translate(temp_at) * mat4::rotate(n.normalize(), -angle) * vec4(temp_eye, 1));
		vec4 update_up = (mat4::rotate(n.normalize(), -angle) * vec4(temp_up, 1));

		c->eye = vec3(update_eye.x, update_eye.y, update_eye.z);
		c->up = vec3(update_up.x, update_up.y, update_up.z);

		return mat4::look_at(c->eye, c->at, c->up);
	}

	mat4 zoom(float x, float y, camera* c) {
		// retrive the current mouse position
		vec2 m = vec2(x, y) * 2.0f - 1.0f; // normalize xy

		float amount = m0.y - m.y;
		c->eye = temp_eye + (temp_at - temp_eye) * amount;
		return mat4::look_at(c->eye, c->at, c->up);
	}

	mat4 pan(float x, float y, camera* c) {
		// retrive the current mouse position
		vec2 m = vec2(x, y) * 2.0f - 1.0f; // normalize xy

		// project a 2D mouse position to a unit sphere
		vec3 p1 = vec3(m.x - m0.x, m0.y - m.y, 0);	// displacement with vertical swap
		if (!b_tracking || length(p1) < 0.0001f) return view_matrix0;			// ignore subtle movement


		vec3 a = c->eye - c->at;
		a = a.normalize();
		vec3 b = cross(c->up, a);
		b = b.normalize();
		vec3 c2 = cross(a, b);
		c2 = c2.normalize();

		c->eye = temp_eye - (p1.x * b + p1.y * c2) * 150.f;
		c->at = temp_at - (p1.x * b + p1.y * c2) * 150.f;

		return mat4::look_at(c->eye, c->at, c->up);
	}
};

#endif // __TRACKBALL_H__
