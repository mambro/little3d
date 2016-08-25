
#include "little3d/little3d.hpp"

// stupid repeated cube
std::array<GLfloat,36*3> g_vertex_buffer_data = {
    -1.0f,-1.0f,-1.0f, // triangle 1 : begin
    -1.0f,-1.0f, 1.0f,
    -1.0f, 1.0f, 1.0f, // triangle 1 : end
    1.0f, 1.0f,-1.0f, // triangle 2 : begin
    -1.0f,-1.0f,-1.0f,
    -1.0f, 1.0f,-1.0f, // triangle 2 : end
    1.0f,-1.0f, 1.0f,
    -1.0f,-1.0f,-1.0f,
    1.0f,-1.0f,-1.0f,
    1.0f, 1.0f,-1.0f,
    1.0f,-1.0f,-1.0f,
    -1.0f,-1.0f,-1.0f,
    -1.0f,-1.0f,-1.0f,
    -1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f,-1.0f,
    1.0f,-1.0f, 1.0f,
    -1.0f,-1.0f, 1.0f,
    -1.0f,-1.0f,-1.0f,
    -1.0f, 1.0f, 1.0f,
    -1.0f,-1.0f, 1.0f,
    1.0f,-1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
    1.0f,-1.0f,-1.0f,
    1.0f, 1.0f,-1.0f,
    1.0f,-1.0f,-1.0f,
    1.0f, 1.0f, 1.0f,
    1.0f,-1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
    1.0f, 1.0f,-1.0f,
    -1.0f, 1.0f,-1.0f,
    1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f,-1.0f,
    -1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f, 1.0f,
    1.0f,-1.0f, 1.0f
};

using namespace little3d;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_E && action == GLFW_PRESS)
    {
    }
}


const char * meshv = GLSL330(
	layout(location = 0) in vec3 point;
	uniform mat4 T;

	void main(){
	    gl_Position =  T*vec4(point,1.0);
	}
);

const char * meshf = GLSL330(

	out vec4 color;

	void main()
	{
		color = vec4(1.0, 0.0, 0.0,1);
	}
);

int main(int argc, char **argv)
{
	int width = 640;
	int height = 480;
	auto window = little3d::init(width,height);
	
	VBO<1> vvbo;
	VAO vao;
	Shader sha(Shader::FromString,meshv,meshf);

	{
		GLScope<VBO<1> > t(vvbo);
		t.set(g_vertex_buffer_data);

		GLScope<VAO> v(vao);
	 	glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	}

	glfwSetKeyCallback(*window, key_callback);

	Eigen::Matrix4f bm;
	bm.setIdentity();
	auto Proj = little3d::eigen::perspective<float>(60.0f,         // The horizontal Field of View, in degrees : the amount of "zoom". Think "camera lens". Usually between 90° (extra wide) and 30° (quite zoomed in)
	    window->innerRatio, // Aspect Ratio. Depends on the size of your window. Notice that 4/3 == 800/600 == 1280/960, sounds familiar ?
	    0.1f,        // Near clipping plane. Keep as big as possible, or you'll get precision issues.
	    300.0f       // Far clipping plane. Keep as little as possible.
	);
	auto View      = little3d::eigen::lookAt<float>({0,5,5},{0,0,0},{0,1,0});

	ArcBall hb(Eigen::Vector3f(0,0,0),0.75,window->screenToNDC());
	hb.attach(window);

	glERR("opengl:load");

	do {
		window->clear();

		{
			GLScope<VAO> va(vao);
			GLScope<Shader> sh(sha);
			sha.uniform<Eigen::Matrix4f>("T") << Proj*View*hb.getTransformation()*bm;
			glDrawArrays(GL_TRIANGLES, 0, g_vertex_buffer_data.size()  / 3);
		}

		glfwSwapBuffers(*window);
		glfwPollEvents();
	} 
	while( glfwGetKey(*window, GLFW_KEY_ESCAPE ) != GLFW_PRESS && glfwWindowShouldClose(*window) == 0 );
	glfwTerminate();
}