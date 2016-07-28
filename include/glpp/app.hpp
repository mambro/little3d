#pragma once
#ifdef USE_EGL
#define GL_GLEXT_PROTOTYPES
#include "EGL/egl.h"
#include "GLES2/gl2.h"
#else
#include <glew.h>
#endif
#include <GLFW/glfw3.h>
#include <memory>
#include <Eigen/Dense>
#include <functional>
#include <iostream>

namespace glpp
{

struct GLSize
{
	GLSize() {}

	GLSize(int w, int h) : width(w),height(h) {}

	bool empty() const { return width == 0 && height == 0; }

	bool singular() const { return width == 1 || height == 1; }

	bool operator != (const GLSize & o) const { return !(o == *this); }
	bool operator == (const GLSize & o) const { return o.width == width && o.height == height; }

	int width = 0,height = 0;
};



inline std::ostream & operator << (std::ostream & ons, const GLSize & x)
{
	ons << "glsize[" << x.width << " " << x.height << "]";
	return ons;
}


inline int pow2roundup (int x)
{
	int result = 1;
	while (result < x) result <<= 1;
	return result;
}

inline GLSize nextpow2(GLSize sz)
{
	return GLSize(pow2roundup(sz.width),pow2roundup(sz.height));
}

	struct XGLFWwindow
	{
		GLFWwindow * window = 0;
		GLSize windowSize;
		GLSize viewportSize;
		int innerWidth,innerHeight;
		float devicePixelRatio;
		float innerRatio;
		float ppmm; // pixerl per millimeter

		operator GLFWwindow* () { return window; }

		Eigen::Matrix4f screenToNDC() 
		{
			Eigen::Matrix4f r;
			r << 2.0/innerWidth,0,0,-1,
				0,2.0/innerHeight,0,-1,
				0,0,1.0,0,
				0,0,0,1;
			return r;
		}

		std::function<void(GLFWwindow *,int button, int action, int mods)> mousefx;
		std::function<void(GLFWwindow*,double x, double y)> movefx;

		void bind()
		{
			glfwSetMouseButtonCallback(window,[] (GLFWwindow* window, int button, int action, int mods)
			{
					XGLFWwindow * p = (XGLFWwindow*)glfwGetWindowUserPointer(window);
					if(p->mousefx)
						p->mousefx(window,button,action,mods);
			});
			glfwSetCursorPosCallback(window,[] (GLFWwindow* window,double x, double y)
			{
					XGLFWwindow * p = (XGLFWwindow*)glfwGetWindowUserPointer(window);
					if(p->movefx)
						p->movefx(window,x,y);
			});
		}

	};

	inline  std::shared_ptr<XGLFWwindow> init(int width,int height, const char * title = "tmp", bool visible = true)
	{
		using T = XGLFWwindow;
		if( !glfwInit() )
		{
			return std::shared_ptr<T>(0);
		}

		//glfwWindowHint(GLFW_SAMPLES, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
		glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

		glfwWindowHint(GLFW_VISIBLE, visible ? GL_TRUE : GL_FALSE);

		GLFWwindow* window = glfwCreateWindow( width, height, title, NULL, NULL);
		if( window == NULL )
		{
			glfwTerminate();
			return std::shared_ptr<T>(0);
		}

		glfwMakeContextCurrent(window);

#ifndef USE_EGL
		glewExperimental = true; // Needed for core profile
		if (glewInit() != GLEW_OK) 
		{
			return std::shared_ptr<T>(0);
		}
#endif
		int q[4];
		glGetIntegerv(GL_VIEWPORT,q);

		//glERR("glew:init");

		// Ensure we can capture the escape key being pressed below
		// Ensure we can capture the escape key being pressed below
		glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

		// Dark blue background

		// Enable depth test
		glEnable(GL_DEPTH_TEST);
		// Default: glDepthFunc(GL_LESS); 
		glEnable(GL_CULL_FACE);
	 
		glClearColor(0.0f, 0.0f, 0.2f, 0.0f);	
		auto monitor = glfwGetWindowMonitor(window);
		T r;
		r.window = window;
		r.innerWidth = width;
		r.innerHeight = height;
		r.innerRatio = width/(float)height;
		r.viewportSize = GLSize(q[2],q[3]);
		std::cout << "app window " << r.innerWidth << "x" << r.innerHeight << " viewport " << q[2] << "x" << q[3] << std::endl; 

		int widthMM, heightMM;
		if(monitor)
		{
			glfwGetMonitorPhysicalSize(monitor, &widthMM, &heightMM);
			const GLFWvidmode* mode = glfwGetVideoMode(monitor);
			if(mode)
			{
				const double ppmm = mode->width / (float)widthMM; // pixel per mm
				r.ppmm = ppmm;
			}
		}

		std::shared_ptr<T> rr = std::make_shared<T>(r);
		glfwSetWindowUserPointer(window,rr.get());
		rr->bind();// link

		//glEnable(GL_BLEND);
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


		//r.devicePixelRatio = 0;
		return rr;
	}

}