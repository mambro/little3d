#pragma once

#define GLSL150(src) "#version 150 core\n" #src
#define GLSL330(src) "#version 330\n" #src
#define GLSL(src) #src


//#include "SOIL.h"
#include <stdio.h>
#include <string>
#include <stdlib.h> // memory management
#include <string>
#include <vector>
#include <array>
#include <iostream>
#include <fstream>
#include <algorithm>
#ifdef BOOST_LOG
#include <boost/log/trivial.hpp>
#else
#define BOOST_LOG_TRIVIAL(x) std::cout
#endif
#include "little3d/ioutils.hpp"
#include "little3d/base.hpp"
#include <Eigen/Dense>


namespace little3d
{
template <class T>
class WrappedUniform
{
public:
};

/**
 * Simple Shader clase
 */
class Shader
{
public:
	enum LoadSource { FromFile, FromString};

	class ShaderException : public std::exception 
	{

	};

	Shader() {}
	operator GLuint ()
	{
		return resource_;
	}

	Shader(LoadSource s,const char * vertex_file_path,const char * fragment_file_path,const char * geo_file_path=0)
	{
		load(vertex_file_path,fragment_file_path,geo_file_path,0,0,s == FromFile);
	}

	~Shader()
	{
		release();
	}

	void release()
	{
		if(resource_)
		{
			glDeleteProgram(resource_);
			resource_ = 0;
		}
	}

	void bind()
	{
		glUseProgram(resource_);
	}

	void unbind()
	{
		glUseProgram(0);
	}

	GLint attributeLocation(const char * name)
	{
		return glGetAttribLocation(resource_,name);
	}


	GLint uniformLocation(const char * name, bool hard = true)
	{
		GLint r = glGetUniformLocation(resource_,name);
		if(r < 0 && hard)
		{
			std::cerr << "failed glGetUniformLocation " << name << std::endl;
			throw ShaderException();
		}
		else
			return r;
	}

	bool load(const char * vertex_file_path,const char * fragment_file_path,const char * geo_file_path=0, const GLchar**captures=0, int ncaptures=0, bool fromfile=true) ;

	template <class T>
	WrappedUniform<T> uniform(const char * name)
	{
		return WrappedUniform<T>(glGetUniformLocation(resource_,name));
	}

	template <class T>
	WrappedUniform<T> uniform(const std::string & x)
	{
		return WrappedUniform<T>(glGetUniformLocation(resource_,x.c_str()));
	}

	template <class T>
	WrappedUniform<T> uniform(int i)
	{
		return WrappedUniform<T>(i);
	}

private:
	Shader(const Shader & );
	GLuint resource_ = 0;
};


class WrappedUniformBase
{
public:
	WrappedUniformBase(int n): uloc(n) {}

	int uloc;

	void init(Shader & s, const char * name, bool needed = true)
	{
		uloc = s.uniformLocation(name,needed);
	}
};

std::ostream & operator << ( std::ostream & ons, WrappedUniformBase & x)
{
	ons << "ULOC:"<<x.uloc;
	return ons;
}


template <>
class WrappedUniform<Eigen::Vector4f>: public WrappedUniformBase
{
public:
	WrappedUniform(int n=-1): WrappedUniformBase(n) {}

	void operator<<(const Eigen::Vector4f & x)
	{
		*this = x;
	}
	WrappedUniform& operator=(const Eigen::Vector4f & x)
	{
		// TODO assert about binding
		glUniform4fv(uloc,1,x.data());
		return *this;
	}
	
};
template <>
class WrappedUniform<Eigen::Vector3f>: public WrappedUniformBase
{
public:
	WrappedUniform(int n=-1): WrappedUniformBase(n) {}

	void operator<<(const Eigen::Vector4f & x)
	{
		*this = x;
	}

	WrappedUniform& operator=(const Eigen::Vector4f & x)
	{
		// TODO assert about binding
		glUniform3fv(uloc,1,x.data());
		// GL_INVALID_OPERATION
		return *this;
	}
};

template <>
class WrappedUniform<float>: public WrappedUniformBase
{
public:
	WrappedUniform(int n=-1): WrappedUniformBase(n) {}

	WrappedUniform& operator=(const float & x)
	{
		glUniform1f(uloc,x);
		return *this;
	}

	void operator<<(const float & x)
	{
		*this = x;
	}
};


template <>
class WrappedUniform<int>: public WrappedUniformBase
{
public:
	WrappedUniform(int n=-1): WrappedUniformBase(n) {}

	WrappedUniform& operator=(const int & x)
	{
		glUniform1i(uloc,x);
		return *this;
	}

	void operator<<(const int & x)
	{
		*this = x;
	}
};

template <>
class WrappedUniform<Eigen::Matrix4f >: public WrappedUniformBase
{
public:
	WrappedUniform(int n=-1): WrappedUniformBase(n) {}

	void operator<<(const Eigen::Matrix4f  & x)
	{
		// TODO assert about binding
		glUniformMatrix4fv(uloc,1,GL_FALSE,x.data());
	}
};


template <>
class WrappedUniform<Eigen::Matrix3f >: public WrappedUniformBase
{
public:
	WrappedUniform(int n=-1): WrappedUniformBase(n) {}

	void operator<<(const Eigen::Matrix3f  & x)
	{
		// TODO assert about binding
		glUniformMatrix3fv(uloc,1,GL_FALSE,x.data());
	}
};


/**
 * This code requires cleanup
 */
inline bool Shader::load(const char * vertex_file_path,const char * fragment_file_path,const char * geo_file_path, const GLchar**captures, int ncaptures, bool fromfile) {

	GLuint programID = 0;

	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = 0;
	GLuint GeoShaderID = 0;
	if(fragment_file_path != 0 && fragment_file_path[0] != 0)
		FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);
#ifndef USE_EGL	
	if(geo_file_path != 0 && geo_file_path[0] != 0)
		GeoShaderID = glCreateShader(GL_GEOMETRY_SHADER);
#endif

	// Read the Vertex Shader code from the file
	std::string VertexShaderCode;
	if(fromfile)
	{
		std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
		if(VertexShaderStream.is_open()){
			std::string Line = "";
			while(getline(VertexShaderStream, Line))
				VertexShaderCode += "\n" + Line;
			VertexShaderStream.close();
		}else{
			return false;
		}
	}
	else
		VertexShaderCode = vertex_file_path;

	// Read the Fragment Shader code from the file
	std::string FragmentShaderCode;
	if(FragmentShaderID)
	{
		if(fromfile)
		{
			std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
			if(FragmentShaderStream.is_open()){
				std::string Line = "";
				while(getline(FragmentShaderStream, Line))
					FragmentShaderCode += "\n" + Line;
				FragmentShaderStream.close();
			}
		}
		else
		{
			FragmentShaderCode = fragment_file_path;	
		}
	}

	// Read the Fragment Shader code from the file
	std::string GeoShaderCode;
	if(GeoShaderID)
	{
		if(fromfile)
		{
			std::ifstream GeoShaderStream(geo_file_path, std::ios::in);
			if(GeoShaderStream.is_open()){
				std::string Line = "";
				while(getline(GeoShaderStream, Line))
					GeoShaderCode += "\n" + Line;
				GeoShaderStream.close();
			}
		}
		else
		{
			GeoShaderCode = geo_file_path;	
		}
	}

	GLint Result = GL_FALSE;
	int InfoLogLength;

	// Compile Vertex Shader
	//printf("Compiling shader : %s\n", fromfile ? vertex_file_path: "<inline>");
	char const * VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if ( Result == 0){
		std::vector<char> VertexShaderErrorMessage(InfoLogLength+1);
		glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
		printf("failed %s\n", &VertexShaderErrorMessage[0]);
		return false;
	}

	if(FragmentShaderID)
	{
		// Compile Fragment Shader
		//printf("Compiling shader : %s\n", fromfile? fragment_file_path : "<inline>");
		char const * FragmentSourcePointer = FragmentShaderCode.c_str();
		glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
		glCompileShader(FragmentShaderID);

		// Check Fragment Shader
		glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
		glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
		if ( Result == 0 ){
			std::vector<char> FragmentShaderErrorMessage(InfoLogLength+1);
			glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
			printf("failed %s\n", &FragmentShaderErrorMessage[0]);
			return false;
		}
	}

	if(GeoShaderID)
	{
		const char * file_path = geo_file_path;
		std::string & code = GeoShaderCode;
		GLuint & sid = GeoShaderID;

		printf("Compiling shader : %s\n",fromfile? file_path:"<inline>");

		char const * SourcePointer = code.c_str();
		glShaderSource(sid, 1, &SourcePointer , NULL);
		glCompileShader(sid);
		glGetShaderiv(sid, GL_COMPILE_STATUS, &Result);
		glGetShaderiv(sid, GL_INFO_LOG_LENGTH, &InfoLogLength);
		if ( Result == 0 ){
			std::vector<char> ErrorMessage(InfoLogLength+1);
			glGetShaderInfoLog(GeoShaderID, InfoLogLength, NULL, &ErrorMessage[0]);
			printf("failed %s\n", &ErrorMessage[0]);
			return false;
		}
	}

	programID = glCreateProgram();
	glAttachShader(programID, VertexShaderID);
	glAttachShader(programID, FragmentShaderID);
	if(GeoShaderID)
		glAttachShader(programID, GeoShaderID);
#ifndef USE_EGL
	if(ncaptures)
	{
		glTransformFeedbackVaryings(programID, ncaptures, captures, GL_INTERLEAVED_ATTRIBS);		
	}
#endif
	glLinkProgram(programID);

	glGetProgramiv(programID, GL_LINK_STATUS, &Result);
	glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if ( Result == GL_FALSE)
	{
		std::vector<char> ProgramErrorMessage(InfoLogLength+1);
		glGetProgramInfoLog(programID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
		printf("%s\n", &ProgramErrorMessage[0]);
		return false;
	}

	glDeleteShader(VertexShaderID);
	if(FragmentShaderID)
		glDeleteShader(FragmentShaderID);
	if(GeoShaderID)
		glDeleteShader(GeoShaderID);

	release();
	resource_ = programID;
	return true;
}

	/**
	 * Scope for Shader == glUseProgram
	 */
	template<>
	struct GLScope<Shader>
	{
		GLScope(Shader & x) { x.bind(); }
		~GLScope() { glUseProgram(0); }
	};

}
