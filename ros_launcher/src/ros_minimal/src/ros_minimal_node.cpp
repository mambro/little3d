#include <ros/ros.h>
#include <glew.h>
#include <GLFW/glfw3.h>
#include "std_msgs/String.h"
#include "std_msgs/Empty.h"
#include "glpp/draw.hpp"
#include "glpp/gleigen.hpp"
#include "glpp/imageproc.hpp"

void mySigintHandler(int sig)
{
  // Do some custom action.
  // For example, publish a stop message to some other nodes.
  
  // All the default sigint handler does is call shutdown()
  ros::shutdown();
}

bool requestPending = false;
bool visible = true;
std::string requestImage;

bool projectImage(std_msgs::String & req, std_msgs::Empty & res)
{
	requestPending = true;
	requestImage = req.data;
	return true;
}

bool switchoff(std_msgs::Empty & req, std_msgs::Empty & res)
{
	visible = false;
	return true;
}

int main(int argc, char  *argv[])
{
	bool visible = true;
	int width =640;
	int height=480;
	const char * title = "hello";
	const char * mymonitor = "monitor2";

	if(!glfwInit())
		return -1;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	glfwWindowHint(GLFW_VISIBLE, visible ? GL_TRUE : GL_FALSE);
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
		return -1;
	}
	GLFWwindow* window = glfwCreateWindow( width, height, title, monitor, NULL);

	if(!window)
		return -1;
	glfwMakeContextCurrent(window);
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) 
		return 0;

	glClearColor(0.0,0.0,0.0,1.0);
	 ros::init(argc, argv, "my_node_name");
	 ros::NodeHandle nh;
	 signal(SIGINT, mySigintHandler);

	 ros::ServiceServer service = nh.advertiseService("projectImage", projectImage);
	 ros::ServiceServer service2 = nh.advertiseService("switchoff", switchoff);
		glpp::GLImageProc img;
		img.init();
		// TODO: modify imageproc to support the homography
		glpp::Texture tex;
	do {
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		if(visible)
			img.runOnScreen(tex);
		glfwSwapBuffers(window);
		glfwPollEvents();
		ros::spinOnce(); 
		// requestPending/visible updated after callback
		if(requestPending)
		{
			// load image
			if(!tex.load(requestImage.c_str()))
			{
				// ERROR message 
			}
			requestPending = false;
		}
	}
	while( ros::ok() && glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS && glfwWindowShouldClose(window) == 0 );
	glfwTerminate();
	return 0;
}