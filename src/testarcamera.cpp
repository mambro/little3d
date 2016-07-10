#include "glpp/arcamera.hpp"
#include <iostream>


Eigen::Vector4f divw(Eigen::Vector4f x)
{
	return x/x(3);
}

Eigen::Vector4f scaleopenglviewV(Eigen::Vector4f x,float s)
{
	Eigen::Vector4f r;
	r << x(0)*s,x(1)*s,x(2),x(3);
	return r;
}



int main(int argc, char * argv[])
{

	// Data from some Aruco marker
	Eigen::Matrix3f K; K << 532.850,0.0,316.880,0.0,532.450,241.630,0.0,0.0,1.0;
	Eigen::Matrix4f glp; glp << 1.665156173706055,-0.0,0.0,0.0,0.0,-2.662250061035156,0.0,0.0,0.009749984741210915,0.2081500244140626,-1.002002002002002,-1.0,0.0,-0.0,-0.2002002002002002,0.0;
	glp.transposeInPlace();
	float n=0.1,f=100;
	int w=640,h=400;
	Eigen::Vector3f t; t << 0.01256639789789915,0.03324272111058235,0.4833259284496307;
	Eigen::Matrix4f mv; mv << -0.05342024937272072,0.8759374022483826,0.4794579446315765,0.0,0.9959275722503662,0.08165632933378220,-0.03821634873747826,0.0,0.07262590527534485,-0.47546386718750,0.8767323493957520,0.0,0.01256639324128628,0.03324271738529205,-0.4833259284496307,1.0;
	mv.transposeInPlace();
	Eigen::Vector2f c; c << 330.401855468750,280.7149658203125;
	Eigen::Matrix4f pose; pose << -0.05342030525207520,0.9959275722503662,0.07262593507766724,0.01256639789789915,0.8759374618530273,0.08165638893842697,-0.4754637479782104,0.03324272111058235,-0.4794578552246094,0.03821635618805885,-0.8767324090003967,0.4833259284496307,0.0,0.0,0.0,1.0;

	// Extract CI
	CameraIntrinsics ci;
	ci.width = w;
	ci.height = h;
	ci.fromK(K);
	Eigen::Matrix4f K4 = ci.getK4(n,f);
	float glscale = 2.0;
	float gw= glscale*w;
	float gh=glscale*h;

	Eigen::Matrix4f myglp = GLclip2viewport(gw,gh).inverse()*GLflipviewport(gh).inverse()*GLscaleViewport(glscale)*K4*GLworld2eye().inverse();
	Eigen::Matrix4f myglp1 = ci.getGLProjection(n,f,true);

	std::cout << "aruco K\n" << K <<std::endl;
	std::cout << "aruco K4\n" << K4 <<std::endl;
	std::cout << "aruco glp\n" << glp <<std::endl;
	std::cout << "myglp\n" << myglp << std::endl;
	std::cout << "myglp1\n" << myglp1 << std::endl;
	std::cout << "aruco mv\n" << mv <<std::endl;
	std::cout << "aruco pose\n" << pose <<std::endl;
	std::cout << "W * aruco pose\n" << GLworld2eye()*pose <<std::endl;
	std::cout << "clip2view\n" << GLclip2viewport(gw,gh) << std::endl;

	std::cout << "origin2eye " << (GLworld2eye()*pose*Eigen::Vector4f(0,0,0,1)).transpose() <<std::endl;



	// project origin (of pose) to center
	Eigen::Vector4f pAR;
	std::cout << "center                         " << c.transpose()  << std::endl;
	pAR = scaleopenglviewV(divw(K4*pose*Eigen::Vector4f(0,0,0,1)),glscale);
	std::cout << "divw*K4*pose                  " << pAR.transpose() <<std::endl;

	// use provided
	pAR = GLflipviewport(gh)*GLclip2viewport(gw,gh)*divw(glp*mv*Eigen::Vector4f(0,0,0,1));
	std::cout << "aruco_glp*aruco_mv => opengl  " << pAR.transpose()  << std::endl;

	// final flip is due to reversed image from aruco
	pAR = divw(GLflipviewport(gh)*GLclip2viewport(gw,gh)*divw(glp*GLworld2eye()*pose*Eigen::Vector4f(0,0,0,1)));
	std::cout << "aruco_glp*my_mv => opengl     " << pAR.transpose()   << std::endl;

	// final flip is due to reversed image from aruco
	pAR = (GLflipviewport(gh)*GLclip2viewport(gw,gh)*divw(myglp*mv*Eigen::Vector4f(0,0,0,1)));
	std::cout << "my_glp*aruco_mv => opengl     " << pAR.transpose()  << std::endl;

	// final flip is due to reversed image from aruco
	pAR = (GLflipviewport(gh)*GLclip2viewport(gw,gh)*divw(myglp1*mv*Eigen::Vector4f(0,0,0,1)));
	std::cout << "my_glp1*aruco_mv => opengl    " << pAR.transpose()  << std::endl;

	// final flip is due to reversed image from aruco
	pAR = divw(GLflipviewport(gh)*GLclip2viewport(gw,gh)*(myglp1*mv*Eigen::Vector4f(0,0,0,1)));
	std::cout << "my_glp1*aruco_mv => opengl lat" << pAR.transpose()  << std::endl;
	return 0;

}