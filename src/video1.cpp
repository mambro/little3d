#include "glpp/app.hpp"
#include "glpp/draw.hpp"
#include "glpp/gleigen.hpp"
#include "glpp/imageproc.hpp"
#include "glpp/gstreamvideo.hpp"
#include <Eigen/Geometry>
#include <iostream>
#include <chrono>
#include "glpp/timingex.hpp"

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
	
	int width = 640;
	int height = 480;
	auto window = glpp::init(width,height);
	glERR("opengl:after init app");

	GLImageProc img;
	Texture tex;
	GStreamerVideo video;
	if(argc > 1)
	{
		if(argv[1][0] == '!')
			video.pipelinetext_ = argv[1]+1;
		else
		{
			video.setPlayFile(argv[1]);
		}
	}
	video.onConfig();
		glERR("opengl:posttexload");
	img.init();
		glERR("opengl:GLImageProc init");
	std::cout << "loaded " << tex.size() << " " << tex.realsize() << " flip:" << tex.flipped() << " res:" << (int)tex << std::endl;
		glERR("opengl:load");

	// TODO: make it lambda function
	glfwSetKeyCallback(window, key_callback);

	PeriodTiming<> pt2,pt0,pt1;
	do {
		pt0.start();
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		glERR("opengl:prerender");
		if(swap)
		{
			// LT LB RT RB
			std::array<float,8> full({-0.5,1.0,   -0.5,-1.0,  0.5,1.0,   0.5,-1.0 });
	        img.setVerticesClipSpace(full);
	        swap = false;
	        video.pipelinetext_ = "";
	        video.onConfig();
		}
		pt2.start();
		if(video.onUpdate())
		{
			// from video to tex (without PBO)
			if(!tex)
				tex.initcolor(video.size_,GL_RGB,GL_RGB,GL_UNSIGNED_BYTE);
			tex.bind();
	        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, video.size_.width, video.size_.height,
	                     0, GL_RGB, GL_UNSIGNED_BYTE, &video.buffer_[0]);			
			tex.unbind();	
		}
		pt2.stop();
		pt1.start();
        img.runOnScreen(tex);
        pt1.stop();
		glfwSwapBuffers(window);
		glfwPollEvents();
		pt0.stop();
	} 
	while( glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS && glfwWindowShouldClose(window) == 0 );
	std::cout << "Video onUpdate " << pt2 << std::endl;
	std::cout << "Texture upload " << pt1 << std::endl;
	std::cout << "Frame begin-postswap " << pt0 << std::endl;
	glfwTerminate();
}