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

GLFWwindow* winit(int width,int height, const char * mymonitor)
{
		
	const char * title = "projector_screen";

	if(!glfwInit())
	{
		return 0;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
	glfwWindowHint(GLFW_AUTO_ICONIFY, GL_FALSE);

	int n;
	
	GLFWmonitor * monitor = mymonitor != 0 ? 0 : glfwGetPrimaryMonitor();
	auto monitors = glfwGetMonitors(&n);
	for(int i = 0; i < n; i++)
	{
		if(strcmp(glfwGetMonitorName(monitors[i]),mymonitor) == 0)
		{
			monitor = monitors[i];
			break;
		}
	}
	if(!monitor)
	{
		std::cout << "cannot find monitor\n";
		for(int i = 0; i < n; i++)
			std::cout << " " << i << " " << glfwGetMonitorName(monitors[i]) << std::endl;
		return 0;
	}
	
	GLFWwindow * window = glfwCreateWindow( width, height, title, monitor, NULL);

	if(!window)
	{
		std::cout<<"Not window"<<std::endl;
		return 0;
	}

	glfwMakeContextCurrent(window);
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) 
	{
		return 0;
	}
	return window;
}

int main(int argc, char **argv)
{
	if(argc < 2)
	{	
		std::cout << "required: imagename [monitorname]" << std::endl;
		return -1;
	}	
	int width = 1280;
	int height = 800;
	auto window = winit(width,height,"HDMI-0");
	glERR("opengl:after init app");

	GLImageProc img;
	Texture tex;
	if(!tex.load(argv[1]))
		return 0;
		glERR("opengl:posttexload");
	img.init();
		glERR("opengl:GLImageProc init");
	std::cout << "loaded " << tex.size() << " " << tex.realsize() << " flip:" << tex.flipped() << " res:" << (int)tex << std::endl;
		glERR("opengl:load");

	// TODO: make it lambda function
	glfwSetKeyCallback(window, key_callback);


	do {
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		glERR("opengl:prerender");
		if(swap)
		{
			// LT LB RT RB
			std::array<float,8> full({-0.516531,
										0.133457,
										0.726285,
										0.155106,
										0.740697,
	                                    -0.821361,
                                      -0.511139
                                       -0.847909 });		      
			//std::array<float,8> full({-0.5,1.0,   -0.5,-1.0,  0.5,1.0,   0.5,-1.0 });
	        img.setVerticesClipSpace(full);
	        swap = false;
		}
        img.runOnScreen(tex);
		glfwSwapBuffers(window);
		glfwPollEvents();
	} 
	while( glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS && glfwWindowShouldClose(window) == 0 );
	glfwTerminate();
}
