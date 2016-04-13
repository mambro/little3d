#include "glpp/app.hpp"
#include "glpp/draw.hpp"
#include <iostream>

using namespace glpp;

const float SQUARE[] = {
    -1.0f,  1.0f,
    -1.0f, -1.0f,
     1.0f,  1.0f,
     1.0f, -1.0f
};


const char * meshv = GLSL330(
	layout(location = 0) in vec2 point;
	uniform float angle;

	void main(){
		mat2 rotate = mat2(cos(angle), -sin(angle),sin(angle), cos(angle));
	    gl_Position =  vec4(0.75 * rotate * point, 0.0, 1.0);
	}
);

const char * meshf = GLSL330(
	out vec4 color;

	void main()
	{
		color = vec4(1.0, 0.15, 0.15,0);
	}
);

int main(int argc, char **argv)
{
	float angle = 0.0;
	int width = 640;
	int height = 480;
	auto window = glpp::init(width,height);
		glERR("opengl:after init");

	VBO<1> vvbo;
	VAO vao;
	vvbo.init();
	vao.init();
	Shader sha;
	std::cout << "vao " << vao << " vbo " << vvbo << std::endl;
	{
	    sha.load(meshv, meshf, 0, 0, 0, false);
	}
	{
		GLScope<VBO<1> > t(vvbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(SQUARE), SQUARE, GL_STATIC_DRAW);
	}
	{
		glERR("opengl:pre vao setup");
		GLScope<VAO> v(vao);
		glERR("opengl:vao bind");
		GLScope<VBO<1>> p(vvbo);
		glERR("opengl:vbo bind");
	 	glEnableVertexAttribArray(0);
		glERR("opengl:enable attrib");
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glERR("opengl:vertex attrib");
	}

	std::cout << "shader is " << sha << std::endl;
	int uniform_angle = glGetUniformLocation(sha, "angle");
	std::cout << "uniform is " << uniform_angle << "!=" << GL_INVALID_VALUE << std::endl;

	do {
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

		{
			angle += 0.2;
			GLScope<VAO> va(vao);
			GLScope<Shader> sh(sha);
			glERR("opengl:scoppe");
			glUniform1f(uniform_angle, angle);
			glERR("opengl:uniform");
			glDrawArrays(GL_TRIANGLE_STRIP, 0, sizeof(SQUARE)/sizeof(SQUARE[0]) / 2);
			glERR("opengl:draw");
		}

		glfwSwapBuffers(window);
		glfwPollEvents();
	} 
	while( glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS && glfwWindowShouldClose(window) == 0 );
	glfwTerminate();
}