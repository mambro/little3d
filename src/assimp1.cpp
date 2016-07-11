#include "glpp/app.hpp"
#include "glpp/draw.hpp"
#include "glpp/gleigen.hpp"
#include "glpp/imageproc.hpp"
#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags
#include <Eigen/Geometry>
#include <iostream>
#include "glpp/ArcBall.hpp"
#include "assimpex.hpp"

using namespace glpp;


int main(int argc, char **argv)
{
	if(argc != 2)
	{
		std::cerr << "needed filename of assimp resource\n";
		return -1;
	}
	int width = 640;
	int height = 480;
	auto window = glpp::init(width,height);

	std::vector<std::unique_ptr<basicobj> >  objects;
	std::shared_ptr<material> mat = std::make_shared<material>();
	assimploader(argv[1],objects,mat);

	// custom shader
	{
	    if(!mat->sha.load(StandardShader::meshv, StandardShader::meshf, 0, 0, 0, false))
	    	exit(-1);
	    GLScope<Shader> ss(mat->sha);
	    mat->initshader();
		glERR("opengl:setup");
	}

	// view
	ArcBall hb(Eigen::Vector3f(0,0,0),0.75,window->screenToNDC());

	std::cout << "go...\n";
	auto Proj = glpp::eigen::perspective<float>(60.0f,         // The horizontal Field of View, in degrees : the amount of "zoom". Think "camera lens". Usually between 90° (extra wide) and 30° (quite zoomed in)
	    width/(float)height, // Aspect Ratio. Depends on the size of your window. Notice that 4/3 == 800/600 == 1280/960, sounds familiar ?
	    0.1f,        // Near clipping plane. Keep as big as possible, or you'll get precision issues.
	    300.0f       // Far clipping plane. Keep as little as possible.
	);
	auto View      = glpp::eigen::lookAt<float>({0,2,2},{0,0,0},{0,1,0});
	Eigen::Matrix4f Model = Eigen::Matrix4f::Identity();
	Model.block<3,3>(0,0) = Eigen::Matrix3f::Identity();

	std::cout << "Proj is\n" << Proj  << std::endl;
	std::cout << "View is\n" <<  View  << std::endl;
	std::cout << "Model is\n" <<  Model << std::endl;
	std::cout << "Matrix is\n" << Proj * View * Model << std::endl;

	matrixsetup ms;
	ms.V = View;
	ms.M = Model;
	ms.P = Proj;
	GLImageProc imgproc;
	imgproc.init();

	window->movefx = [&hb] (GLFWwindow *w,double x, double y) 
	{
		if(glfwGetMouseButton(w,0) == GLFW_PRESS)
		{
			hb.drag(Eigen::Vector2f(x,y));
		}
	};

	window->mousefx = [&hb] (GLFWwindow *w,int button, int action, int mods) 
	{
		if(button == 0 && action == GLFW_PRESS)
		{
			double p[2];
			glfwGetCursorPos(w,&p[0],&p[1]); // make helper
			hb.beginDrag(Eigen::Vector2f(p[0],p[1]));
		}		
	};

	do {
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		ms.M = hb.getTransformation();
		for(auto & o : objects)
			o->render(ms);
		glfwSwapBuffers(*window);
		glfwPollEvents();
	} 
	while( glfwGetKey(*window, GLFW_KEY_ESCAPE ) != GLFW_PRESS && glfwWindowShouldClose(*window) == 0 );
	glfwTerminate();
}