
#include "glpp/app.hpp"
#include "glpp/draw.hpp"
#include "glpp/gleigen.hpp"
#include "glpp/imageproc.hpp"
#include "glpp/offline.hpp"
#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags
#include <Eigen/Geometry>
#include <iostream>
#include "glpp/ArcBall.hpp"
#include "assimpex.hpp"
#include "glpp/arcamera.hpp"

using namespace glpp;

int main(int argc, char **argv)
{
	if(argc != 2)
	{
		std::cerr << "needed filename of assimp resource\n";
		return -1;
	}
	bool capturemode = true;
	int width = 640;
	int height = 480;
	auto window = glpp::init(width,height);

	std::vector<std::unique_ptr<basicobj> >  objects;
	std::vector<std::shared_ptr<material> > mats;
	assimploader(argv[1],objects,mats);

	std::cout << "assimploader loaded.\n";

	// custom shader
	for(int i = 0; i < mats.size(); i++)
	{
		auto& mat = mats[i];
		mat->sha = std::make_shared<Shader>();
	    if(!mat->sha->load(StandardShader::meshv, StandardShader::meshf, 0, 0, 0, false))
	    	exit(-1);
		std::cout << "shader loaded.\n";
	    GLScope<Shader> ss(*mat->sha.get());
	    mat->initshader();
		glERR("opengl:setup");
	std::cout << "material loaded.\n";
	}
	
	// view
	//ArcBall hb(Eigen::Vector3f(0,0,0),0.75,window->screenToNDC());

	std::cout << "go...\n";

	Eigen::Matrix3f K;
	K << 483.88531161999998, 0, 285.0078125,0, 483.41334710000001, 190.57286071799999,0,0,1;  //  [  ], [  ], [ 0, 0, 1 ]
	CameraIntrinsics ci;
	ci.width = width;
	ci.height = height;
	ci.fromK(K);
	Eigen::Matrix4f Proj = ci.getGLProjection(0.1f,300.0f,true);
	Eigen::Matrix4f Pose;
	Pose << -0.89118343591690063, -0.42827165126800543, 0.14958447217941279, -0.17440372705459589, -0.35403132438659668, 0.45042487978935242, -0.81962144374847412, 0.048705145716667182,0.28364405035972601, -0.7833905816078186, -0.55303275585174561, 0.95084923505783081,0,0,0,1;
	/*
	"glmodelview": [
        -0.8911834359169,
        -0.3540313243866,
        -0.28364405035973,
        0,
        -0.42827165126801,
        0.45042487978935,
        0.78339058160782,
        0,
        0.14958447217941,
        -0.81962144374847,
        0.55303275585175,
        0,
        -0.1744037270546,
        0.048705141991377,
        -0.95084923505783,
        1
      ],
	*/
	Eigen::Matrix4f View  = Eigen::Matrix4f::Identity();
	Eigen::Matrix4f Model = Pose;
	
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

	GLImageProc img;
	Texture tex;
	if(!tex.load("2015-12-18T11_08_29Z__4_left.png"))
		return 0;
	img.init();
	std::cout << "loaded " << tex.size() << " " << tex.realsize() << " flip:" << tex.flipped() << " res:" << (int)tex << std::endl;

	ColorTexture trgb;
	DepthTexture tdepth;
	trgb.init(window->viewportSize,true); 
	tdepth.init(window->viewportSize,false); // float

	FBO fbo;
	{
		FBO::Setup s(fbo);
		s.attach(trgb);
		s.attach(tdepth);
		// optional: call glDrawBuffers pointing to GL_COLOR_ATTACHMENT0
	}


	do {
		GLViewportScope view;
		if(capturemode)
		{
			fbo.bind(GL_FRAMEBUFFER);
		}
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        img.runOnScreen(tex);
		for(auto & o : objects)
			o->render(ms);
		glfwSwapBuffers(*window);
		if(capturemode)
		{
			glBindFramebuffer(GL_FRAMEBUFFER,0); 
			// imgflip.getOutput().save

			if(!trgb.save("color.png"))
				std::cout << "failed saveccolor\n";
			if(!tdepth.save("depth.png"))
				std::cout << "failed save depth\n";
			break;
		}
		glfwPollEvents();
	} 
	while( glfwGetKey(*window, GLFW_KEY_ESCAPE ) != GLFW_PRESS && glfwWindowShouldClose(*window) == 0 );

	glfwTerminate();
}