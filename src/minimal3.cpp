#include "glpp/app.hpp"
#include "glpp/draw.hpp"
#include "glpp/gleigen.hpp"
#include <Eigen/Geometry>
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


	/*Set up a perspective projection matrix
	Like the deprecated gluPerspective function
	*/
	//TODO ArcBall ab(glm::vec3(0,0,0),0.75,);
	auto Proj = glpp::eigen::perspective<float>(60.0f,  // The horizontal Field of View, in degrees : the amount of "zoom". 
														//Think "camera lens". Usually between 90° (extra wide) and 30° (quite zoomed in)
	    width/(float)height, // Aspect Ratio. Depends on the size of your window. Notice that 4/3 == 800/600 == 1280/960, sounds familiar ?
	    0.1f,        // Near clipping plane. Keep as big as possible, or you'll get precision issues.
	    300.0f       // Far clipping plane. Keep as little as possible.
	);


	/*Depth buffer precision is affected by the values specified for zNear and zFar.
	The greater the ratio of zFar to zNear is,the less effective the depth buffer will be at distinguishing between
	surfaces that are near each other.	If   r= zFar / zNear, we have roughly that log2(r) bits of depth
	buffer precision are lost. */

	/*  LOOKAT
	To place the virtual camera to capture the view.
	Syntax like the deprecated gluLookAt()
	1) The point EYE (ex, ey, ez) defines the location of the camera.
	2) The vector AT (ax, ay, az) denotes the direction where the camera is aiming at, 
		usually at the center of the world or an object.
	3) The vector UP (ux, uy, uz) denotes the upward orientation of the camera roughly. 
	   UP is typically coincided with the y-axis of the world space. UP is roughly orthogonal to AT, but not necessary.
	   As UP and AT define a plane, we can construct an orthogonal vector to AT in the camera space.
	*/

	auto View      = glpp::eigen::lookAt<float>({0,2,20},{0,0,0},{0,1,0});

	Eigen::Matrix4f Model = Eigen::Matrix4f::Identity();
	Model.block<3,3>(0,0) = 10*Eigen::Matrix3f::Identity();
	std::cout << "Proj is\n" << Proj  << std::endl;
	std::cout << "View is\n" <<  View  << std::endl;
	std::cout << "Model is\n" <<  Model << std::endl;
	std::cout << "Matrix is\n" << Proj * View * Model << std::endl;


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

		Eigen::Matrix4f  MVP        = Proj * View * Model; 
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

		glfwSwapBuffers(*window);
		glfwPollEvents();
	} 
	while( glfwGetKey(*window, GLFW_KEY_ESCAPE ) != GLFW_PRESS && glfwWindowShouldClose(*window) == 0 );
	glfwTerminate();
}