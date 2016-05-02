#include <glew.h>
#include <GLFW/glfw3.h>



int main(int argc, char const *argv[])
{
	bool visible = true;
	int width =640;
	int height=480;
	const char * title = "hello";

	if(!glfwInit())
		return -1;
	// The following  three lines ensure to use  the proper OpenGL version, not older version than 3.3 
	// with programmable shaders.
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	// For MAC OS X to initialize the code
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	// The prevent window resizing
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
	// Starts the created window visible or unvisible
	glfwWindowHint(GLFW_VISIBLE, visible ? GL_TRUE : GL_FALSE);
	// Window creation
	GLFWwindow* window = glfwCreateWindow( width, height, title, NULL, NULL);

	if(!window)
		return -1;
	// We tell GLFW to make the window context the main context on the current thread
	glfwMakeContextCurrent(window);
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) 
		return 0;
	do 
	{
		// It clears al every iteration the color buffer
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		// glfwSwapBuffers swaps between the two image buffers (front baffer , actually displayed,
		// and back buffer, that will be displayed)
		glfwSwapBuffers(window);
		// glfwPollEvents checks if any events are triggered (keyboard input, mouse movements) 
		glfwPollEvents();
	}
	while( glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS && glfwWindowShouldClose(window) == 0 );
	// It Ends properly all the resources
	glfwTerminate();
	return 0;
}