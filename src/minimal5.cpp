#include "glpp/app.hpp"
#include "glpp/draw.hpp"
#include "glpp/gleigen.hpp"
#include "glpp/imageproc.hpp"
#include <Eigen/Geometry>
#include <iostream>

using namespace glpp;
#define COCO_ERR() std::cout
float angle = 0;

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
	img.init();
	if(argc > 2)
		tex.save(argv[2]);
	std::cout << "loaded " << tex.size() << " " << tex.realsize() << "  " << tex.flipped() << " res:" << (int)tex << std::endl;
		glERR("opengl:load");

		glERR("opengl:initimg");
		img.runOnScreen(tex);
		glERR("opengl:render");

	do {
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        img.runOnScreen(tex);
		glfwSwapBuffers(window);
		glfwPollEvents();
	} 
	while( glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS && glfwWindowShouldClose(window) == 0 );
	glfwTerminate();
}