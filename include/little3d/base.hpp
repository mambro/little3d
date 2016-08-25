/**
 * OpenGL 3.3+ and some OpenGL ES 2.0+ helpers in C++
 *
 * Emanuele Ruffaldi 2014-2016
 */
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
#include <array>

#include "little3d/strictgl.hpp"
#include "little3d/ioutils.hpp"
#ifdef BOOST_LOG
#include <boost/log/trivial.hpp>
#else
#define BOOST_LOG_TRIVIAL(x) std::cout
#endif


/// dumps the OpenGL error with text
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


namespace little3d
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

/// A wrapper for image formats in OpenGL taken from libfreenect2, enhanced with Channels
template<size_t TBytesPerPixel, GLenum TInternalFormat, GLenum TFormat, GLenum TType, size_t TChannels>
struct ImageFormat
{
    static const size_t BytesPerPixel = TBytesPerPixel;
    static const GLenum InternalFormat = TInternalFormat;
    static const GLenum Format = TFormat;
    static const GLenum Type = TType;
    static const int Channels = TChannels;
};

#ifndef USE_EGL
typedef ImageFormat<1, GL_R8UI, GL_RED_INTEGER, GL_UNSIGNED_BYTE,1> U8C1;
typedef ImageFormat<2, GL_R16I, GL_RED_INTEGER, GL_SHORT,1> S16C1;
typedef ImageFormat<2, GL_R16UI, GL_RED_INTEGER, GL_UNSIGNED_SHORT,1> U16C1;
typedef ImageFormat<4, GL_R32F, GL_RED, GL_FLOAT,1> F32C1;
typedef ImageFormat<8, GL_RG32F, GL_RG, GL_FLOAT,2> F32C2;
typedef ImageFormat<12, GL_RGB32F, GL_RGB, GL_FLOAT,3> F32C3;
typedef ImageFormat<16, GL_RGBA32F, GL_RGBA, GL_FLOAT,4> F32C4;
#endif
typedef ImageFormat<4, GL_RGBA, GL_BGRA, GL_UNSIGNED_BYTE,4> F8C4;



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

		Query()
		{
			init();
		}

		void init()
		{
			if(resource_)
				release();
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
		VBO() { init(); }

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
		VBO(): resource_(0) {  init(); }

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

	/**
	 * Dual Pixel Buffer Object used for Multi-thread transfers
	 */
	class DualPBO
	{
	public:
		DualPBO(): index(0)
		{
			init(); 
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

	/**
	 * Multiple PBO with fences
	 */
	template <int N>
	class MultiPBOFenced
	{
	public:
		MultiPBOFenced(): index(0)
		{
			memset(fences,0,sizeof(fences));	
			init();
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
		VAO(): resource_(0) { init(); }

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

	//----------------------------------------------------------------------------
	// Scope Based Tools
	//----------------------------------------------------------------------------


	/**
	 * Scoped Viewport with automatic reset
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
	 * Scope for VAO == glBindVertexArray
	 */
	template<>
	struct GLScope<VAO>
	{
		GLScope(VAO & x) { x.bind(); }
		~GLScope() { glBindVertexArray(0); }
	};



	/**
	 * Scope for VBO<n> == glBindBuffer
	 */
	template<int n>
	struct GLScope<VBO<n> >
	{
		GLScope(VBO<n> & x, GLenum mode = GL_ARRAY_BUFFER, int unit = 0) : mode_(mode) { x.bind(mode,unit); }
		~GLScope() { glBindBuffer(mode_,0); }

		template <class T,unsigned long N>
		void set(const std::array<T,N> & w)
		{
			glBufferData(GL_ARRAY_BUFFER, w.size()*sizeof(T),w.data(),GL_STATIC_DRAW);
		}

		template <class T>
		void set(const std::vector<T> & w)
		{
			glBufferData(GL_ARRAY_BUFFER, w.size()*sizeof(T),&w[0],GL_STATIC_DRAW);
		}

		GLenum mode_;
	};

	/**
	 * Scope for VBO<1>
	 */
	template<>
	struct GLScope<VBO<1> >
	{
		GLScope(VBO<1> & x, GLenum mode = GL_ARRAY_BUFFER): mode_(mode) { x.bind(mode); }
		~GLScope() { glBindBuffer(mode_,0); }

		template <class T,unsigned long N>
		void set(const std::array<T,N> & w)
		{
			glBufferData(GL_ARRAY_BUFFER, w.size()*sizeof(T),w.data(),GL_STATIC_DRAW);
		}

		template <class T>
		void set(const std::vector<T> & w)
		{
			glBufferData(GL_ARRAY_BUFFER, w.size()*sizeof(T),&w[0],GL_STATIC_DRAW);
		}


		GLenum mode_;
	};

	/**
	 * Enables a variable with restore in scope
	 */
	template <GLenum x>
	struct GLScopeEnable
	{
		GLScopeEnable() { glEnable(x); }
		~GLScopeEnable() { glDisable(x); }
	};

	/**
	 * Disable a variable with restore in scope
	 */
	template <GLenum x>
	struct GLScopeDisable
	{
		GLScopeDisable() { glDisable(x); }
		~GLScopeDisable() { glEnable(x); }
	};

	/**
	 * Specialization for Depth writing
	 */
	template <>
	struct GLScopeDisable<GL_DEPTH_WRITEMASK>
	{
		GLScopeDisable() { glDepthMask(GL_FALSE); }
		~GLScopeDisable() { glDepthMask(GL_TRUE); }
	};

	/**
	 * Specialization for Color writing
	 */
	template <>
	struct GLScopeDisable<GL_COLOR_WRITEMASK>
	{
		GLScopeDisable() { glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE); }
		~GLScopeDisable() { glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE); }
	};



	/// helper to specify front side rendering
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

}