#pragma once

#define GLSL150(src) "#version 150 core\n" #src
#define GLSL330(src) "#version 330\n" #src
#define GLSL(src) #src

//#include "SOIL.h"
#include <stdio.h>
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
#include "glpp/ioutils.hpp"


inline bool glERR(const char * name)
{
	//BOOST_LOG_NAMED_SCOPE("GL");
	bool r = true;
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
    	const char * e = err == GL_INVALID_VALUE ? "invalidvalue" : err == GL_INVALID_OPERATION ? "invalidop " : "";
        BOOST_LOG_TRIVIAL(warning) << "OpenGL error: " << name << " " << err << " " << e<< std::endl;
        r = false;
    }
    return r;
}

namespace glpp
{

///////////////////////////////////////////////////////////////////////////////
// check FBO completeness
///////////////////////////////////////////////////////////////////////////////
inline bool checkFramebufferStatus(GLenum status)
{
    switch(status)
    {
    case GL_FRAMEBUFFER_COMPLETE:
        std::cout << "FBO: Framebuffer complete." << std::endl;
        return true;

    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
        std::cout << "[ERROR] Framebuffer incomplete: Attachment is NOT complete." << std::endl;
        return false;

    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
        std::cout << "[ERROR] Framebuffer incomplete: No image is attached to FBO." << std::endl;
        return false;
/*
    case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
        std::cout << "[ERROR] Framebuffer incomplete: Attached images have different dimensions." << std::endl;
        return false;

    case GL_FRAMEBUFFER_INCOMPLETE_FORMATS:
        std::cout << "[ERROR] Framebuffer incomplete: Color attached images have different internal formats." << std::endl;
        return false;
*/
    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
        std::cout << "[ERROR] Framebuffer incomplete: Draw buffer." << std::endl;
        return false;

    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
        std::cout << "[ERROR] Framebuffer incomplete: Read buffer." << std::endl;
        return false;

    case GL_FRAMEBUFFER_UNSUPPORTED:
        std::cout << "[ERROR] Framebuffer incomplete: Unsupported by FBO implementation." << std::endl;
        return false;

    default:
        std::cout << "[ERROR] Framebuffer incomplete: Unknown error." << std::endl;
        return false;
    }
}
//     // check FBO status
//    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

inline int
pow2roundup (int x)
{
	int result = 1;
	while (result < x) result <<= 1;
	return result;
}

/// from libfreenect2
template<size_t TBytesPerPixel, GLenum TInternalFormat, GLenum TFormat, GLenum TType>
struct ImageFormat
{
    static const size_t BytesPerPixel = TBytesPerPixel;
    static const GLenum InternalFormat = TInternalFormat;
    static const GLenum Format = TFormat;
    static const GLenum Type = TType;
};

typedef ImageFormat<1, GL_R8UI, GL_RED_INTEGER, GL_UNSIGNED_BYTE> U8C1;
typedef ImageFormat<2, GL_R16I, GL_RED_INTEGER, GL_SHORT> S16C1;
typedef ImageFormat<2, GL_R16UI, GL_RED_INTEGER, GL_UNSIGNED_SHORT> U16C1;
typedef ImageFormat<4, GL_R32F, GL_RED, GL_FLOAT> F32C1;
typedef ImageFormat<8, GL_RG32F, GL_RG, GL_FLOAT> F32C2;
typedef ImageFormat<12, GL_RGB32F, GL_RGB, GL_FLOAT> F32C3;
typedef ImageFormat<4, GL_RGBA, GL_BGRA, GL_UNSIGNED_BYTE> F8C4;
typedef ImageFormat<16, GL_RGBA32F, GL_RGBA, GL_FLOAT> F32C4;


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


inline GLSize nextpow2(GLSize sz)
{
	return GLSize(pow2roundup(sz.width),pow2roundup(sz.height));
}


class Texture
{
public:
	void bind(int unit = 0,GLenum what = GL_TEXTURE_2D)
	{
		glActiveTexture(GL_TEXTURE0 + unit);
		glBindTexture(what,resource_);
	}

	void unbind(int unit = 0,GLenum what = GL_TEXTURE_2D)
	{
		glActiveTexture(GL_TEXTURE0 + unit);
		glBindTexture(what,0);
	}

	bool load(const char * name, bool expanded = false);

	bool valid() const { return resource_ != 0; }

	operator GLuint () const { return resource_; }

	~Texture()
	{
		release();
	}


	void release()
	{
		if(resource_)
		{
			glDeleteTextures(1,&resource_);
			resource_ = 0;
		}
	}

	void initdepth(GLSize size)
	{
		release();	
		size_ = size;
		glGenTextures(1, &resource_);
		glBindTexture(GL_TEXTURE_2D, resource_);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, size_.width, size_.height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glBindTexture(GL_TEXTURE_2D,0);
		format_ = GL_DEPTH_COMPONENT;
		type_ = GL_FLOAT;
		itemsize_ = 4; // float
	}

	// basic full volume GL_R8 GL_UNSIGNED_BYTE
	bool init3d(const uint8_t * data, int xd,int yd,int zd)
	{
		std::array<int,3> bigsize = {xd,yd,zd};
		return init3d(data,xd*yd*zd,bigsize,bigsize,false,false,true,true);
	}

	/// nbytes is the amount of data (only for check)
	/// size is the size of the 3D texture
	/// extent is the extent of the region to be copied at origin
	/// rgba means GL_RGBA vs GL_R
	/// float means GL_FLOAT vs GL_UNSIGNED_BYTE
	bool init3d(const uint8_t * data, int nbytes, const std::array<int,3> &size, const std::array<int,3>  &extent, bool rgba,bool isfloat, bool clampme, bool interp);

	
	void init(GLSize size, GLenum targetformat, GLenum datatype, bool aspow2 = false)
	{
		release();	
		size_ = size;
		if(aspow2)
		{
			size = nextpow2(size);
		}
		realsize_ = size;

		glGenTextures(1, &resource_);

		glBindTexture(GL_TEXTURE_2D, resource_);
		glTexImage2D(GL_TEXTURE_2D, 0, targetformat, size.width,size.height, 0, targetformat, datatype, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		//glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		//glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glBindTexture(GL_TEXTURE_2D,0);
		itemsize_ = targetformat == GL_RGBA||targetformat == GL_DEPTH_COMPONENT ? 4 : 3;
		format_ = targetformat;
		type_ = GL_UNSIGNED_BYTE;
	}


	void init(GLSize size, bool rgba = true, bool aspow2 = false)
	{
		init(size,(GLenum)(rgba ? GL_RGBA : GL_RGB),GL_UNSIGNED_BYTE,aspow2);
	}

	void resize(GLSize size, GLenum targetformat, bool aspow2 = false)
	{
		size_ = size;
		if(aspow2)
		{
			size = nextpow2(size);
		}
		realsize_ = size;

		glBindTexture(GL_TEXTURE_2D, resource_);
		glTexImage2D(GL_TEXTURE_2D, 0, targetformat, size.width,size.height, 0, targetformat, GL_UNSIGNED_BYTE, NULL);
		glBindTexture(GL_TEXTURE_2D,0);
		itemsize_ = targetformat == GL_RGBA ? 4 : 3;
		format_ = targetformat;
		type_ = GL_UNSIGNED_BYTE;
	}

	int width() const {return size_.width; }
	int height() const {return size_.height; }

	GLSize size() const {return size_;}

	GLSize realsize() const {return realsize_;}
	
	bool flipped() const { return flipped_; }

	void setFlipped(bool f) { flipped_ = f; }

	/// raw means that 
	bool getdataraw(std::vector<uint8_t> & data)
	{
		data.resize(width()*height()*itemsize_);
		glBindTexture(GL_TEXTURE_2D,resource_);
		glGetTexImage(GL_TEXTURE_2D,0,format_,type_,&data[0]);
		glBindTexture(GL_TEXTURE_2D,0);
		return true;
	}

	Texture() {}
private:
	GLenum format_ = 0,type_ = 0;
	int itemsize_ = 4;
	bool flipped_ = false;
	GLSize size_;
	GLSize realsize_;
	GLuint resource_ = 0;

	Texture(const Texture & );
};

/**
 * Simple Shader clase
 */
class Shader
{
public:
	class ShaderException : public std::exception 
	{

	};

	Shader() {}
	operator GLuint ()
	{
		return resource_;
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

	GLint uniformLocation(const char * name, bool hard = true)
	{
		GLint r = glGetUniformLocation(resource_,name);
		if(r < 0 && hard)
			throw ShaderException();
		else
			return r;
	}

	bool load(const char * vertex_file_path,const char * fragment_file_path,const char * geo_file_path=0, const GLchar**captures=0, int ncaptures=0, bool fromfile=true) ;

private:
	Shader(const Shader & );
	GLuint resource_ = 0;
};

/**
 * Query Class
 */
class Query
{
public:
	operator GLuint ()
	{
		return resource_;
	}

	~Query()
	{
		release();
	}

	void init()
	{
		GLuint query;
		glGenQueries(1, &resource_);
	}

	void bind(GLenum mode)
	{
		glBeginQuery(mode,resource_);
	}

	void unbind(GLenum mode)
	{
		glEndQuery(mode);
	}

	void release()
	{
		if(resource_)
		{
			glDeleteQueries(1,&resource_);
			resource_ = 0;
		}
	}

	GLuint query()
	{
		GLuint q = 0;
		glGetQueryObjectuiv(resource_, GL_QUERY_RESULT, &q);
		return q;		
	}


private:
	Query(const Query & );
	GLuint resource_ = 0;
};

/**
 * FBO for OpenGL 3.3
 *
 * NB: MRT in 3.3
   ...
   GLenum targ [3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_NONE, GL_DEPTH_ATTACHMENT };
	glDrawBuffers (4, targ);

 * Note on output and buffers:
 	   glBindFragDataLocation (renderer_1prog, GL_COLOR_ATTACHMENT0, "diffuse_out");
   ALTERNATIVE to layout(location = x)
 */
class FBO
{
public:
	FBO () {}

	~FBO()
	{
		release();
	}

	void init()
	{
		release();
		glGenFramebuffers(1,&resource_);
	}



	operator GLuint ()
	{
		return resource_;
	}

	void release()
	{
		if(resource_)
		{
			glDeleteFramebuffers(1,&resource_);
			resource_ = 0;
		}
		if(rboDepthStencil_)
		{
			glDeleteRenderbuffers(1,&rboDepthStencil_);
			rboDepthStencil_ = 0;
		}
		if(rboColor_)
		{
			glDeleteRenderbuffers(1,&rboColor_);
			rboColor_ = 0;
		}
	}

	/**
	 * Color Attachment from allocated texture
	 */
	void attach(const Texture & t, int index = 0)
	{
		size_ = t.size();
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, GL_TEXTURE_2D, (GLuint)t, 0);		
	}

	/**
	 * Depth Attachment
	 */
	void attachdepth(const Texture & t)
	{
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, (GLuint)t, 0);
	}

	bool makergb()
	{
		if(size_.width == 0)
			return false;

		if(!rboColor_)
			glGenRenderbuffers(1, &rboColor_);
		glBindRenderbuffer(GL_RENDERBUFFER, rboColor_);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8,size_.width,size_.height);
		glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER,rboColor_);		
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
		return true;
	}

	bool makedepth()
	{
		if(size_.width == 0)
			return false;

		if(!rboDepthStencil_)
			glGenRenderbuffers(1, &rboDepthStencil_);
		glBindRenderbuffer(GL_RENDERBUFFER, rboDepthStencil_);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8,size_.width,size_.height);
		glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER,rboDepthStencil_);		
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
		return true;
	}

	enum ReadFormat { ColorRGB, Depth};

	/// raw means that 
	bool getdataraw(std::vector<uint8_t> & data, ReadFormat f)
	{
		GLenum format = GL_RGB,type = GL_UNSIGNED_BYTE;
		int size = 1;
		switch(f)
		{
			case ColorRGB: format = GL_RGB; type = GL_UNSIGNED_BYTE; size = 3; break;
			case Depth: format = GL_DEPTH_COMPONENT; type = GL_FLOAT; size = 4; break;
		}
		data.resize(size_.width*size_.height*size);
		glBindFramebuffer(GL_READ_FRAMEBUFFER,resource_);
		glReadPixels(0,0,size_.width,size_.height,format,type,&data[0]);
		glBindFramebuffer(GL_READ_FRAMEBUFFER,0);
		return true;
	}

	/// raw means that 
	bool getdataraw(std::vector<float> & data, ReadFormat f)
	{
		GLenum format = GL_RGB,type = GL_UNSIGNED_BYTE;
		int size = 1;
		switch(f)
		{
			case ColorRGB: format = GL_RGB; type = GL_FLOAT; size = 3; break;
			case Depth: format = GL_DEPTH_COMPONENT; type = GL_FLOAT; size = 1; break;
		}
		data.resize(size_.width*size_.height*size);
		glBindFramebuffer(GL_READ_FRAMEBUFFER,resource_);
		glReadPixels(0,0,size_.width,size_.height,format,type,&data[0]);
		glBindFramebuffer(GL_READ_FRAMEBUFFER,0);
		return true;
	}

	void checkvalidate()
	{
		checkFramebufferStatus(validate());
	}

	GLenum validate()	
	{
		return glCheckFramebufferStatus(GL_FRAMEBUFFER);
	}

	void bind(GLenum what = GL_FRAMEBUFFER)
	{
		glBindFramebuffer(what,resource_);
	}

	void unbind(GLenum what= GL_FRAMEBUFFER)
	{
		glBindFramebuffer(what,0);
	}

	GLSize size() const { return size_; }

	struct Setup
	{
		FBO & fbo;
		Setup(FBO & f) : fbo(f)
		{
			if(!fbo)
				fbo.init();
			fbo.bind();
		}

		~Setup()
		{
			fbo.unbind();
		}
	};	
private:
	FBO(const FBO & );
	GLSize size_ = {0,0};
	GLuint resource_ = 0;
	GLuint rboDepthStencil_ = 0,rboColor_ = 0;
};


/**
 * Multiple VBOs
 */
template <int N>
class VBO
{
public:
	VBO() { inited_ = false; }

	void init()
	{
		release();
		glGenBuffers (N, resource_);		
		inited_ = true;
	}

/*    glBufferData (
      GL_ARRAY_BUFFER,
      3 * point_count * sizeof (GLfloat),
      points,
      GL_STATIC_DRAW
      */

	void bind(GLenum what  = GL_ARRAY_BUFFER, int i = 0)
	{
	  	glBindBuffer(what,resource_[i]);		
	}

	void unbind(GLenum what = GL_ARRAY_BUFFER)
	{
		glBindBuffer(what,0);
	}

	void release()
	{
		if(inited_)
		{
			glDeleteBuffers(N,resource_);
			inited_ = false;
		}
	}

	GLuint get(int i = 0) { return resource_[i]; }

	~VBO()
	{
		release();
	}
private:
	VBO(const VBO&);
	bool inited_;
	GLuint resource_[N];
};

/**
 * One single VBO
 */
template <>
class VBO<1>
{
public:
	VBO(): resource_(0) {  }

	void init()
	{
		release();
		glGenBuffers (1, &resource_);		
	}

	/// just allocate
	void alloc(int size, GLenum mode = GL_STATIC_READ, GLenum what = GL_ARRAY_BUFFER)
	{
		init();
		bind(what);
		glBufferData(what, size,nullptr, mode);
		unbind(what);	
	}

	void bind(GLenum what, int index = 0)
	{
		if(index == 0)
		  	glBindBuffer(what,resource_);		
	}

	void unbind(GLenum what)
	{
		glBindBuffer(what,0);
	}

	void release()
	{
		if(resource_)
		{
			glDeleteBuffers(1,&resource_);
		}
	}

	operator GLuint ()
	{
		return resource_;
	}

	~VBO()
	{
		release();
	}
private:
	VBO(const VBO&);
	GLuint resource_;
};


inline void memcpyVBO(GLuint dst, GLuint src, int soff, int woff, int size)
{
	glBindBuffer(GL_COPY_READ_BUFFER, dst);
	glBindBuffer(GL_COPY_WRITE_BUFFER, src);
	glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, soff,woff,size);
}

class DualPBO
{
public:
	DualPBO(): index(0)
	{

	}

	void init()
	{
		pbos.init();
	}

	void bindfront()
	{
		pbos.bind(GL_PIXEL_UNPACK_BUFFER_ARB,index);
	}

	void unbind()
	{
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB,0);
	}

	void bindback()
	{
		pbos.bind(GL_PIXEL_UNPACK_BUFFER_ARB,index?0:1);
	}

	void swap()
	{
		index = index ? 0:1;
	}

private:
	int index;
	VBO<2> pbos;
};

template <int N>
class MultiPBOFenced
{
public:
	MultiPBOFenced(): index(0)
	{
		memset(fences,0,sizeof(fences));	
	}

	void init()
	{
		pbos.init();
	}

	void bindfront()
	{
		pbos.bind(GL_PIXEL_UNPACK_BUFFER_ARB,index);
	}

	void unbind()
	{
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB,0);
	}

	void bindback()
	{
		pbos.bind(GL_PIXEL_UNPACK_BUFFER_ARB,(index+1)%N);
	}

	constexpr int count() const { return N; }

	void swap()
	{
		index = (index+1) % N;
	}

	void bind(GLenum what, int i)
	{
		pbos.bind(what,i);
	}

	void deletefront()
	{
		GLsync & front = frontfenceslot();
		glDeleteSync(front);
		front = 0;
	}

	GLsync & backfenceslot() { return fences[(index+1) % N]; }

	GLsync & frontfenceslot() { return fences[index]; }

private:
	int index;
	GLsync fences[N];
	VBO<N> pbos;
};

/**
 * VAO class, one single VAO
 */
class VAO
{
public:
	VAO(): resource_(0) {}

	void init()
	{
		release();
		glGenVertexArrays (1, &resource_);		
	}
	void bind()
	{
	  	glBindVertexArray (resource_);		
	}
	void unbind()
	{
		glBindVertexArray(0);
	}
	operator GLuint ()
	{
		return resource_;
	}
	~VAO()
	{
		release();
	}
	void release()
	{
		if(resource_)
		{
			glDeleteVertexArrays(1,&resource_);
			resource_ = 0;
		}
	}
private:
	VAO(const VAO&);
	GLuint resource_;

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
	if(geo_file_path != 0 && geo_file_path[0] != 0)
		GeoShaderID = glCreateShader(GL_GEOMETRY_SHADER);


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
	if(ncaptures)
	{
		glTransformFeedbackVaryings(programID, ncaptures, captures, GL_INTERLEAVED_ATTRIBS);		
	}
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

inline	bool Texture::load(const char * name, bool expanded)
	{
		release();
		int width;
		int height;
#if 0
		unsigned char * image = SOIL_load_image(name, &width, &height, 0, SOIL_LOAD_RGB);

		if (image == NULL) 
		{
			return false;
		}
		size_ = GLSize(width,height);
		

		/*
		resource_ = SOIL_load_OGL_texture
		(
		  name,
		  SOIL_LOAD_AUTO,
		  SOIL_CREATE_NEW_ID,
		  SOIL_FLAG_INVERT_Y|SOIL_LOAD_RGBA
		  //SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y | SOIL_FLAG_NTSC_SAFE_RGB 
		  //| SOIL_FLAG_COMPRESS_TO_DXT
		);		
		*/

		glGenTextures(1,&resource_);
		bind();

		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glGenerateMipmap(GL_TEXTURE_2D);

		int width2 = pow2roundup(width);
		int height2 = pow2roundup(height);

		if(expanded)
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width2, height2, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0,0, width, height, GL_RGB, GL_UNSIGNED_BYTE, image);
		}
		else
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);			
		}
		
		GLint realwidth = 0;
		GLint realheight = 0;
		glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_WIDTH,&realwidth);
		glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_HEIGHT,&realheight);
		//std::cout << "Texture real " << realwidth << " x " << realheight << " for "  << width << " x " << height << std::endl; 
		SOIL_free_image_data(image);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		float color[] = { 1.0f, 0.0f, 0.0f, 1.0f };
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, color);
		unbind();
		return true;
		#else
		return false;
		#endif
	}

/**
 * Scope for Viewport set and restore
 */
struct GLViewportScope
{
	GLViewportScope(GLint x, GLint y, GLint w, GLint h)
	{
		glGetIntegerv(GL_VIEWPORT,view);
		glViewport(x,y,w,h);
	}
	GLViewportScope(GLSize sz): GLViewportScope(0,0,sz.width,sz.height)
	{
	}

	GLViewportScope()
	{
		glGetIntegerv(GL_VIEWPORT,view);
	}
	~GLViewportScope()
	{
		glViewport(view[0],view[1],view[2],view[3]);
	}

	GLint view[4];
};

/**
 * Generic Scope
 */
template<class T>
struct GLScope
{
	GLScope(T & x, GLenum mode): x_(x),mode_(mode) { x.bind(mode); }

	~GLScope() { x_.unbind(mode_); }

	T & x_;
	GLenum mode_;
};


/**
 * Scope for Shader
 */
template<>
struct GLScope<Shader>
{
	GLScope(Shader & x) { x.bind(); }
	~GLScope() { glUseProgram(0); }
};

/**
 * Scope for VAO
 */
template<>
struct GLScope<VAO>
{
	GLScope(VAO & x) { x.bind(); }
	~GLScope() { glBindVertexArray(0); }
};

/**
 * Scope for VAO
 */
template<>
struct GLScope<FBO>
{	
	GLScope(FBO & x, GLenum mode = GL_FRAMEBUFFER) : _view(x.size()), _mode(mode) { x.bind(mode); 


	}
	~GLScope() { glBindFramebuffer(_mode,0); }

	// for syntax: if(GLScope<FBO> _ = pippo)
	operator bool() { return true; }
	GLViewportScope _view;
	GLenum _mode;
};

/**
 * Scope for VBO<n> with unit specification
 */
template<int n>
struct GLScope<VBO<n> >
{
	GLScope(VBO<n> & x, GLenum mode = GL_ARRAY_BUFFER, int unit = 0): unit_(unit), x_(x),mode_(mode) { x.bind(mode,unit); }
	~GLScope() { x_.unbind(mode_); }

	VBO<n> & x_;
	int unit_;
	GLenum mode_;
};

/**
 * Scope for VBO<n> with unit specification
 */
template<>
struct GLScope<VBO<1> >
{
	GLScope(VBO<1> & x, GLenum mode = GL_ARRAY_BUFFER): x_(x),mode_(mode) { x.bind(mode); }
	~GLScope() { x_.unbind(mode_); }

	VBO<1> & x_;
	GLenum mode_;
};

template <GLenum x>
struct GLScopeEnable
{
	GLScopeEnable() { glEnable(x); }
	~GLScopeEnable() { glDisable(x); }
};

template <GLenum x>
struct GLScopeDisable
{
	GLScopeDisable() { glDisable(x); }
	~GLScopeDisable() { glEnable(x); }
};

/**
 * Scope for Texture with unit specification
 */
template<>
struct GLScope<Texture>
{
	GLScope(Texture & x, GLenum mode, int unit = 0):  mode_(mode) { x.bind(unit,mode);}
	GLScope(GLuint x, GLenum mode, int unit = 0):  mode_(mode) { glActiveTexture(GL_TEXTURE0+unit); glBindTexture(mode,x); }
	~GLScope() { glBindTexture(mode_,0); }

	GLenum mode_;
};


inline void xglFrontSide()
{
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
}

inline void xglDualSide()
{	
	glDisable(GL_CULL_FACE);
}

inline void xglBackSide()
{
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CW);	
}

	inline bool Texture::init3d(const uint8_t * data, int nbytes, const std::array<int,3> &xsize, const std::array<int,3>  &extent, bool rgba,bool isfloat, bool clampme, bool interp)
	{
		release();
		glGenTextures(1, &resource_);

		GLScope<Texture> ss(*this,GL_TEXTURE_3D);
		glERR("opengl:init3dpre bound");
		bool samesize = xsize == extent;
		GLenum type =  isfloat ? GL_FLOAT : rgba ? GL_UNSIGNED_INT_8_8_8_8_REV : GL_UNSIGNED_BYTE;
		 GLenum internalformat = (rgba? (isfloat?GL_RGBA32F:GL_RGBA8): (isfloat? GL_R32F:GL_R8));
		glERR("opengl:init3dpre");
		   glTexImage3D(GL_TEXTURE_3D,0,
		   		// internalformat
		   		internalformat,
		   		xsize[0],xsize[1],xsize[2],
		   		0, // border
		   		rgba ? GL_RGBA:GL_RED, // ignored
		   		type, // ignored
		   		samesize ? data : 0);
		if(!glERR("opengl:init3dpre glTexImage3D"))
			return false;
		if(!samesize)
		{
		   glTexSubImage3D(GL_TEXTURE_3D,0,
		   		0,0,0, // origin
		   		extent[0],extent[1],extent[2], // extent
		   		rgba ? GL_RGBA:GL_RED,
		   		type,
		   		data);
			if(!glERR("opengl:init3dpre glTexSubImage3D"))
			return false;
		}
		   // set the texture parameters
		   glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, clampme ? GL_CLAMP_TO_BORDER: GL_REPEAT);
		   glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, clampme ?  GL_CLAMP_TO_BORDER: GL_REPEAT);
		   glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R,  clampme ? GL_CLAMP_TO_BORDER : GL_REPEAT);
		   glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, interp?GL_LINEAR:GL_NEAREST);
		   glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, interp?GL_LINEAR:GL_NEAREST);
		std::cout << "new tex3d size:" <<  xsize << " initedas:" << extent << " rgba:" << rgba << " float:" << isfloat << " clamp:" << clampme << " interp:"<< interp << std::endl;
		return true;
	}
}