#pragma once
#ifdef USE_EGL
#define GL_GLEXT_PROTOTYPES
#include "EGL/egl.h"
#include "GLES2/gl2.h"
#else
#include <glew.h>
#endif
#include <GLFW/glfw3.h>

namespace glpp
{
	struct XGLFWwindow
	{
		GLFWwindow * window = 0;
		int innerWidth,innerHeight;
		float devicePixelRatio;
		float innerRatio;
	};

	inline  XGLFWwindow  init(int width,int height, const char * title = "tmp", bool visible = true)
	{
		if( !glfwInit() )
		{
			return XGLFWwindow();
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
			return XGLFWwindow();
		}

		glfwMakeContextCurrent(window);

#ifndef USE_EGL
		glewExperimental = true; // Needed for core profile
		if (glewInit() != GLEW_OK) 
		{
			return XGLFWwindow();
		}
#endif
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
		int widthMM, heightMM;
		glfwGetMonitorPhysicalSize(monitor, &widthMM, &heightMM);
		const GLFWvidmode* mode = glfwGetVideoMode(monitor);
		const double ppmm = mode->width / (float)widthMM; // pixel per mm

		XGLFWwindow r;
		r.window = window;
		r.innerWidth = width;
		r.innerHeight = height;
		r.innerRatio = width/(float)height;
		r.ppmm = ppmm;
		//r.devicePixelRatio = 0;
		return r;
	}
}