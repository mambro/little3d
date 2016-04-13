#include "glpp/app.hpp"

int main(int argc, char const *argv[])
{
	GLFWwindow*  window = glpp::init(640,480,"Hello");
	if(!window)
		return 0;

	do {
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	while( glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS && glfwWindowShouldClose(window) == 0 );
	glfwTerminate();
	return 0;
}