#include "glpp/app.hpp"
#include "glpp/draw.hpp"
#include "glpp/gleigen.hpp"
#include "glpp/imageproc.hpp"
#include <Eigen/Geometry>
#include <iostream>

using namespace glpp;
#define COCO_ERR() std::cout
float angle = 0;

bool swap = false;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_E && action == GLFW_PRESS)
    {
    	swap = true;
    }
}

int main(int argc, char **argv)
{
	if(argc < 2)
		return -1;
	
	int width = 640;
	int height = 480;
	auto window = glpp::init(width,height);
	glERR("opengl:after init app");

	GLImageProc img;
	Texture tex;
	if(!tex.load(argv[1]))
		return 0;
		glERR("opengl:posttexload");
	img.init();
		glERR("opengl:GLImageProc init");
	if(argc > 2)
		tex.save(argv[2]);
	std::cout << "loaded " << tex.size() << " " << tex.realsize() << " flip:" << tex.flipped() << " res:" << (int)tex << std::endl;
		glERR("opengl:load");

	// TODO: make it lambda function
	glfwSetKeyCallback(*window, key_callback);


	do {
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		glERR("opengl:prerender");
		if(swap)
		{
			// LT LB RT RB
			std::array<float,8> full({-0.5,1.0,   -0.5,-1.0,  0.5,1.0,   0.5,-1.0 });
	        img.setVerticesClipSpace(full);
	        swap = false;
		}
        img.runOnScreen(tex);
		glfwSwapBuffers(*window);
		glfwPollEvents();
	} 
	while( glfwGetKey(*window, GLFW_KEY_ESCAPE ) != GLFW_PRESS && glfwWindowShouldClose(*window) == 0 );
	glfwTerminate();
}