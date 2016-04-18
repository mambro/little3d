#pragma once

#ifdef USE_EGL
#define GL_GLEXT_PROTOTYPES
#include "EGL/egl.h"
#include "GLES2/gl2.h"
#else
#include <glew.h>
#endif
//#include "SOIL.h"
#include <stdio.h>
#include <stdlib.h> // memory management
#include <string>
#include <memory>
#include <vector>
#include <array>
#include <iostream>
#include <fstream>
#include <algorithm>
#include "glpp/strictgl.hpp"
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

#include "shader.hpp"

namespace glpp
{

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

#ifndef USE_EGL
typedef ImageFormat<1, GL_R8UI, GL_RED_INTEGER, GL_UNSIGNED_BYTE> U8C1;
typedef ImageFormat<2, GL_R16I, GL_RED_INTEGER, GL_SHORT> S16C1;
typedef ImageFormat<2, GL_R16UI, GL_RED_INTEGER, GL_UNSIGNED_SHORT> U16C1;
typedef ImageFormat<4, GL_R32F, GL_RED, GL_FLOAT> F32C1;
typedef ImageFormat<8, GL_RG32F, GL_RG, GL_FLOAT> F32C2;
typedef ImageFormat<12, GL_RGB32F, GL_RGB, GL_FLOAT> F32C3;
typedef ImageFormat<16, GL_RGBA32F, GL_RGBA, GL_FLOAT> F32C4;
#endif
typedef ImageFormat<4, GL_RGBA, GL_BGRA, GL_UNSIGNED_BYTE> F8C4;

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


}

#include "texture.hpp"

#include "offline.hpp"

namespace glpp {
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
 * Multiple VBOs
 */
template <int N>
class VBO
{
public:
	VBO() { }

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
	bool inited_ = false;
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


#ifndef USE_EGL
inline void memcpyVBO(GLuint dst, GLuint src, int soff, int woff, int size)
{
	glBindBuffer(GL_COPY_READ_BUFFER, dst);
	glBindBuffer(GL_COPY_WRITE_BUFFER, src);
	glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, soff,woff,size);
}
#endif

#ifndef USE_EGL
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
#endif
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
#ifndef USE_EGL
		glGenVertexArrays (1, &resource_);		
#endif
	}
	void bind()
	{
#ifndef USE_EGL
	  	glBindVertexArray (resource_);		
#endif
	}
	void unbind()
	{
#ifndef USE_EGL
		glBindVertexArray(0);
#endif
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
#ifndef USE_EGL
			glDeleteVertexArrays(1,&resource_);
#endif
			resource_ = 0;
		}
	}
private:
	VAO(const VAO&);
	GLuint resource_;

};

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
#ifndef USE_EGL
	~GLScope() { glBindVertexArray(0); }
#endif
};

/**
 * Scope for VAO
 */
template<>
struct GLScope<FBO>
{	
	GLScope(FBO & x, GLenum mode = GL_FRAMEBUFFER) : _view(x.size()), _mode(mode) { x.bind(mode); 


	}
#ifndef USE_EGL
	~GLScope() { glBindFramebuffer(_mode,0); }
#endif
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
	GLScope(VBO<n> & x, GLenum mode = GL_ARRAY_BUFFER, int unit = 0) : mode_(mode) { x.bind(mode,unit); }
	~GLScope() { glBindBuffer(mode_,0); }
	GLenum mode_;
};

/**
 * Scope for VBO<n> with unit specification
 */
template<>
struct GLScope<VBO<1> >
{
	GLScope(VBO<1> & x, GLenum mode = GL_ARRAY_BUFFER): mode_(mode) { x.bind(mode); }
	~GLScope() { glBindBuffer(mode_,0); }
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
	GLScope(Texture & x, GLenum mode = GL_TEXTURE_2D, int unit = 0):  mode_(mode) { x.bind(mode,unit);}
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
#ifndef USE_EGL
		 GLenum internalformat = (rgba? (isfloat?GL_RGBA32F:GL_RGBA8): (isfloat? GL_R32F:GL_R8));
#else
		if(isfloat || !rgba)
			return false;
		 GLenum internalformat = GL_RGBA8;
#endif
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
