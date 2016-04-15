#include <ros/ros.h>
#include <glew.h>
#include <GLFW/glfw3.h>


void mySigintHandler(int sig)
{
  // Do some custom action.
  // For example, publish a stop message to some other nodes.
  
  // All the default sigint handler does is call shutdown()
  ros::shutdown();
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

	GLFWwindow* window = glfwCreateWindow( width, height, title, NULL, NULL);

	if(!window)
		return -1;
	glfwMakeContextCurrent(window);
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) 
		return 0;

	 	ros::init(argc, argv, "my_node_name");
	 ros::NodeHandle nh;
signal(SIGINT, mySigintHandler);

	do {
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		glfwSwapBuffers(window);
		glfwPollEvents();
		ros::spinOnce();
	}
	while( ros::ok() && glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS && glfwWindowShouldClose(window) == 0 );
	glfwTerminate();
	return 0;
}