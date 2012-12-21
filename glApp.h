#pragma once

#include <stdexcept>
#include <string>
#include <memory>
#include <array>
#include <vector>
#include <functional>
#include <cmath>
#include <GL/glew.h>
#include <GL/glut.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "common_shader_utils.h"

namespace gl {

class gl_exception : public std::runtime_error {
public:
	gl_exception(std::string const& what_arg) : std::runtime_error(what_arg) {}
};

inline void check_error() {
	GLenum err = glGetError();
	if (err == GL_NO_ERROR) return;
	throw gl_exception("");
}

class buffer {
public:
	buffer() {
		glGenBuffers(1, &gl_buffer_);
		check_error();
	}
	void bind_array_buffer() const {
		glBindBuffer(GL_ARRAY_BUFFER, gl_buffer_);
		check_error();
	}
	void bind_element_array_buffer() const {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl_buffer_);
	}
	~buffer() {
		glDeleteBuffers(1, &gl_buffer_);
		check_error();
	}
	operator GLuint() const {
		return gl_buffer_;
	}
private:
	GLuint gl_buffer_;
};

typedef glm::vec3 vec3;

struct vertex {
	vec3 coord;
	vec3 normal;
};

typedef std::vector<vertex> shape;

const double PI = 4 * std::atan(1);

inline shape create_sphere(int slices, int stacks) {
	std::vector<vertex> grids;
	std::vector<vertex> ret;
	for (int i = 0; i <= slices; ++i) {
		for (int j = 0; j <= stacks; ++j) {
			double alpha = 2 * PI * i / slices;
			double beta = PI * j / stacks;
			vec3 coord;
			coord[0] = (GLfloat)(std::sin(beta) * cos(alpha));
			coord[1] = (GLfloat)(std::sin(beta) * sin(alpha));
			coord[2] = (GLfloat)(std::cos(beta));
			vertex v;
			v.coord = coord;
			v.normal = coord;
			grids.push_back(v);
		}
	}
	for (int i = 0; i < slices; ++i) {
		for (int j = 0; j < stacks; ++j) {
			ret.push_back(grids[i * (stacks + 1) + j]);
			ret.push_back(grids[i * (stacks + 1) + j + 1]);
			ret.push_back(grids[(i + 1) * (stacks + 1) + j]);
			ret.push_back(grids[(i + 1) * (stacks + 1) + (j + 1)]);
			ret.push_back(grids[(i + 1) * (stacks + 1) + j]);
			ret.push_back(grids[i * (stacks + 1) + j + 1]);
		}
	}
	return ret;
}

inline std::vector<vertex> create_triangle(vec3 coord0, vec3 coord1, vec3 coord2) {
	glm::vec3 v0(coord0[0], coord0[1], coord0[2]);
	glm::vec3 v1(coord1[0], coord1[1], coord1[2]);
	glm::vec3 v2(coord2[0], coord2[1], coord2[2]);
	glm::vec3 v = glm::cross(v1 - v0, v2 - v0);
	vertex vv;
	vv.normal[0] = v[0];
	vv.normal[1] = v[1];
	vv.normal[2] = v[2];
	vv.coord = coord0;
	std::vector<vertex> ret;
	ret.push_back(vv);
	vv.coord = coord1;
	ret.push_back(vv);
	vv.coord = coord2;
	ret.push_back(vv);
	return ret;
}

template<typename T> 
inline void insert_all(T& cont, T const& other) {
	cont.insert(cont.end(), other.begin(), other.end());
}

inline std::vector<vertex> create_tetra(std::array<vec3, 4> coords) {
	std::vector<vertex> ret;
	insert_all(ret, create_triangle(coords[0], coords[1], coords[2]));
	insert_all(ret, create_triangle(coords[0], coords[2], coords[3]));
	insert_all(ret, create_triangle(coords[0], coords[3], coords[1]));
	insert_all(ret, create_triangle(coords[3], coords[2], coords[1]));
	return ret;
}

inline void transform_shape(std::vector<vertex>& vertices, glm::mat4 const& m) {
	glm::mat3 m_inv_transp(glm::inverseTranspose(m));
	for (vertex& v : vertices) {
		auto vv = m * glm::vec4(v.coord[0], v.coord[1], v.coord[2], 1);
		v.coord[0] = vv[0] / vv[3];
		v.coord[1] = vv[1] / vv[3];
		v.coord[2] = vv[2] / vv[3];
		auto nn = m_inv_transp * glm::vec3(v.normal[0], v.normal[1], v.normal[2]);
		v.normal[0] = nn[0];
		v.normal[1] = nn[1];
		v.normal[2] = nn[2];
	}
}

class program {
public:
	program(char const* vertex_shader_filename, char const* fragment_shader_filename) 
		: gl_program_(glCreateProgram()) 
	{
		GLuint vs, fs;
		if ((vs = create_shader(vertex_shader_filename, GL_VERTEX_SHADER)) == 0) {
			throw gl_exception("load vertex shader failed");
		}
		if ((fs = create_shader(fragment_shader_filename, GL_FRAGMENT_SHADER)) == 0) {
			throw gl_exception("load fragment shader failed");
		}
		glAttachShader(gl_program_, vs);
		glAttachShader(gl_program_, fs);
		glLinkProgram(gl_program_);
		GLint link_ok = GL_FALSE;
		glGetProgramiv(gl_program_, GL_LINK_STATUS, &link_ok);
		if (!link_ok) {
			print_log(gl_program_);
			throw gl_exception("glLinkProgram");
		}
	}
	void use_program() const {
		glUseProgram(gl_program_);
		check_error();
	}

	GLint get_attribute_location(char const* attribute_name) const {
		GLint ret = glGetAttribLocation(gl_program_, attribute_name);
		if (ret == -1) throw gl_exception("Get attribute location fail!");
		check_error();
		return ret;
	}
	GLint get_uniform_location(char const* uniform_name) const {
		GLint ret = glGetUniformLocation(gl_program_, uniform_name);
		if (ret == -1) throw gl_exception("Get uniform location fail!");
		check_error();
		return ret;
	}

	~program() {
		glDeleteProgram(gl_program_);
		check_error();
	}
	operator GLuint() const {
		return gl_program_;
	}
private:
	GLuint gl_program_;
};

class vertex_attrib_array_guard {
public:
	vertex_attrib_array_guard(GLint gl_attribute) : gl_attribute_(gl_attribute) {
		glEnableVertexAttribArray(gl_attribute_);
		check_error();
	}
	~vertex_attrib_array_guard() {
		glDisableVertexAttribArray(gl_attribute_);
		check_error();
	}
private:
	GLint gl_attribute_;
};

namespace detail {

	inline int& win_width_inst() {
		static int instance;
		return instance;
	}

	inline int& win_height_inst() {
		static int instance;
		return instance;
	}


	inline void onReshape(int width, int height) {
		win_width_inst() = width;
		win_height_inst() = height;
		glViewport(0, 0, win_width_inst(), win_height_inst());
	}

	class demo {
	public:
		void display() {
			glClearColor(0.1, 0.2, 0.35, 1.0);
			glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
			prog.use_program();
			{
				vbo_vertices.bind_array_buffer();
				glBufferData(GL_ARRAY_BUFFER, vertices_.size() * sizeof(vertex), vertices_.data(), GL_STATIC_DRAW);
				vertex_attrib_array_guard attrib_v_coord_guard(attrib_v_coord3d);
				vertex_attrib_array_guard attrib_v_normal_guard(attrib_v_normal);
				glVertexAttribPointer(attrib_v_coord3d, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), 0);
				glVertexAttribPointer(attrib_v_normal, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (GLvoid const*)sizeof(vec3));
				glDrawArrays(GL_TRIANGLES, 0, vertices_.size());
			}
			glutSwapBuffers();
		}
		demo(std::function<std::vector<vertex> (double)> get_world) 
			: prog("lighting.v.glsl", "lighting.f.glsl") 
			, get_world_(get_world)
		{
			attrib_v_coord3d = prog.get_attribute_location("v_coord3d");
			attrib_v_normal = prog.get_attribute_location("v_normal");
			uniform_m = glGetUniformLocation(prog, "m");
			uniform_v = glGetUniformLocation(prog, "v");
			uniform_p = glGetUniformLocation(prog, "p");
			uniform_m_3x3_inv_transp = glGetUniformLocation(prog, "m_3x3_inv_transp");
			uniform_v_inv = glGetUniformLocation(prog, "v_inv");
		}
		void onIdle() {
			float angle = glutGet(GLUT_ELAPSED_TIME) / 1000.0 * 45;  // 45° per second
			glm::vec3 axis_y(0, 1, 0);
			glm::mat4 anim = glm::rotate(glm::mat4(1.0f), angle, axis_y);
			glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0, 0.0, 0.0));
			glm::mat4 view = glm::lookAt(glm::vec3(0.0, 0.0, 5.0), glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
			glm::mat4 projection = glm::perspective(45.0f, 1.0f*win_width_inst()/win_height_inst(), 0.1f, 100.0f);
			model = model * anim;
			prog.use_program();
			glUniformMatrix4fv(uniform_m, 1, GL_FALSE, glm::value_ptr(model));
			glUniformMatrix4fv(uniform_v, 1, GL_FALSE, glm::value_ptr(view));
			glUniformMatrix4fv(uniform_p, 1, GL_FALSE, glm::value_ptr(projection));
			glm::mat3 m_3x3_inv_transp(glm::inverseTranspose(model));
			glm::mat4 v_inv(glm::inverse(view));
			glUniformMatrix3fv(uniform_m_3x3_inv_transp, 1, GL_FALSE, glm::value_ptr(m_3x3_inv_transp));
			glUniformMatrix4fv(uniform_v_inv, 1, GL_FALSE, glm::value_ptr(v_inv));
			check_error();
			vertices_ = get_world_(glutGet(GLUT_ELAPSED_TIME) / 1000.0);
			glutPostRedisplay();
		}
	private:
		buffer vbo_vertices;
		program prog;
		GLint attrib_v_coord3d;
		GLint attrib_v_normal;
		GLint uniform_m;
		GLint uniform_v;
		GLint uniform_p;
		GLint uniform_m_3x3_inv_transp;
		GLint uniform_v_inv;
		std::function<std::vector<vertex> (double)> get_world_;
		std::vector<vertex> vertices_;
	};

	demo* pd;

	void onIdle() {
		pd->onIdle();
	}

	inline void onDisplay() {
		pd->display();
	}

}

inline void gl_main(
		int argc, char* argv[], 
		int win_width, int win_height,
		std::function<std::vector<vertex> (double)>&& get_world)
{

	using namespace detail;

	win_width_inst() = win_width;
	win_height_inst() = win_height;

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA|GLUT_ALPHA|GLUT_DOUBLE|GLUT_DEPTH);
	glutInitWindowSize(win_width, win_height);
	glutCreateWindow("OpenGL");

	GLenum glew_status = glewInit();
	if (glew_status != GLEW_OK) throw gl_exception(reinterpret_cast<char const*>(glewGetErrorString(glew_status)));
	if (!GLEW_VERSION_2_0) throw gl_exception("Error: your graphic card does not support OpenGL 2.0\n");

	demo d(get_world);
	pd = &d;
	glutDisplayFunc(onDisplay);
	glutReshapeFunc(onReshape);
	glutIdleFunc(onIdle);
	glEnable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glutMainLoop();

}

}
