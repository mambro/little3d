
#include "little3d/little3d.hpp"

// stupid repeated cube
std::array<GLfloat,36*3> g_vertex_buffer_data = {
    -0.5f,-0.5f,-0.5f, // triangle 1 : begin
    -0.5f,-0.5f, 0.5f,
    -0.5f, 0.5f, 0.5f, // triangle 1 : end
    0.5f, 0.5f,-0.5f, // triangle 2 : begin
    -0.5f,-0.5f,-0.5f,
    -0.5f, 0.5f,-0.5f, // triangle 2 : end
    0.5f,-0.5f, 0.5f,
    -0.5f,-0.5f,-0.5f,
    0.5f,-0.5f,-0.5f,
    0.5f, 0.5f,-0.5f,
    0.5f,-0.5f,-0.5f,
    -0.5f,-0.5f,-0.5f,
    -0.5f,-0.5f,-0.5f,
    -0.5f, 0.5f, 0.5f,
    -0.5f, 0.5f,-0.5f,
    0.5f,-0.5f, 0.5f,
    -0.5f,-0.5f, 0.5f,
    -0.5f,-0.5f,-0.5f,
    -0.5f, 0.5f, 0.5f,
    -0.5f,-0.5f, 0.5f,
    0.5f,-0.5f, 0.5f,
    0.5f, 0.5f, 0.5f,
    0.5f,-0.5f,-0.5f,
    0.5f, 0.5f,-0.5f,
    0.5f,-0.5f,-0.5f,
    0.5f, 0.5f, 0.5f,
    0.5f,-0.5f, 0.5f,
    0.5f, 0.5f, 0.5f,
    0.5f, 0.5f,-0.5f,
    -0.5f, 0.5f,-0.5f,
    0.5f, 0.5f, 0.5f,
    -0.5f, 0.5f,-0.5f,
    -0.5f, 0.5f, 0.5f,
    0.5f, 0.5f, 0.5f,
    -0.5f, 0.5f, 0.5f,
    0.5f,-0.5f, 0.5f
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
	uniform vec4 color;
	out vec4 outcolor;

	void main()
	{
		outcolor = color;
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

	GLImageProc img;
	Texture tex;
	if(!tex.load("2015-12-18T11_08_29Z__4_left.png"))
		return 0;
	std::cout << "loaded " << tex.size() << " " << tex.realsize() << " flip:" << tex.flipped() << " res:" << (int)tex << std::endl;


	glfwSetKeyCallback(*window, key_callback);

	Eigen::Matrix4f bm;
	float markersize = 0.09;
	bm.setIdentity();
	bm.diagonal() = Eigen::Vector4f(markersize,markersize,markersize,1); // scales the cube size 
	bm(2,3) = markersize/2;

	Eigen::Matrix3f K;
	K << 483.88531161999998, 0, 285.0078125,0, 483.41334710000001, 190.57286071799999,0,0,1;  //  [  ], [  ], [ 0, 0, 1 ]

	CameraIntrinsics ci(width,height,K);
	Eigen::Matrix4f Proj = ci.getGLProjection(0.1f,300.0f);	
	Eigen::Matrix4f CameraPose; // transformation from world to camera (inverse camera pose wrt world)
	CameraPose << -0.89118343591690063, -0.42827165126800543, 0.14958447217941279, -0.17440372705459589, -0.35403132438659668, 0.45042487978935242, -0.81962144374847412, 0.048705145716667182,0.28364405035972601, -0.7833905816078186, -0.55303275585174561, 0.95084923505783081,0,0,0,1;
	CameraPose = Eigen::Matrix4f(CameraPose.inverse()); // we use pose of Camera wrt Origin, the above value is the POSITION of the martker wrt Camera
	Eigen::Matrix4f View = little3d::eigen::camerapose2eye(CameraPose);


	std::cout << "Projection is\n" << Proj <<std::endl;
	std::cout << "View is \n" << View << std::endl;
	std::cout << "InvCameraPose is \n" << CameraPose << std::endl;
	std::cout << "CameraPose is \n" << CameraPose.inverse() << std::endl;

	glERR("opengl:load");
	glClearColor(0,0,0,1);
	do {
		window->clear();
        img.runOnScreen(tex);

		{
			GLScope<VAO> va(vao);
			GLScope<Shader> sh(sha);
			sha.uniform<Eigen::Matrix4f>("T") << Proj*View*bm;
			sha.uniform<Eigen::Vector4f>("color") << Eigen::Vector4f(1.0,0.0,0.5,1.0);
			glDrawArrays(GL_TRIANGLES, 0, g_vertex_buffer_data.size() / 3);
		}

		glfwSwapBuffers(*window);
		glfwPollEvents();
	} 
	while( glfwGetKey(*window, GLFW_KEY_ESCAPE ) != GLFW_PRESS && glfwWindowShouldClose(*window) == 0 );
	glfwTerminate();
}