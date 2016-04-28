#include <ros/ros.h>
#include <glew.h>
#include <GLFW/glfw3.h>
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

void projectImage(std_msgs::String img)
{
	requestPending = true;
	requestImage = img;
}

void switchoff()
{
	visible = false;
}

int main(int argc, char  *argv[])
{
	bool visible = true;
	int width =640;
	int height=480;
	const char * title = "hello";

	if(!glfwInit())
		return -1;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	glfwWindowHint(GLFW_VISIBLE, visible ? GL_TRUE : GL_FALSE);
	int n;
	GLFWmonitor * monitor = glfwGetPrimaryMonitor();
	auto monitors = glfwGetMonitors(&n);
	for(int i = 0; i < n; i++)
	{
		if(strcmp(glfwGetMonitorName(monitors[i]),mymonitor) == 0)
		{
			monitor = monitors[i];
			break;
		}
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
		GLImageProc img;
		Texture tex;
	do {
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		if(visible)
			img.runOnScreen(tex);
		glfwSwapBuffers(window);
		glfwPollEvents();
		ros::spinOnce();
		if(requestPending)
		{
			// load image
			if(tex.load(requestImage.c_str())
			{

			}
			requestPending = false;
		}
	}
	while( ros::ok() && glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS && glfwWindowShouldClose(window) == 0 );
	glfwTerminate();
	return 0;
}