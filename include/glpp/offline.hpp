#pragma once

// Capture texture as offline in FBO
namespace glpp {


///////////////////////////////////////////////////////////////////////////////
// check FBO completeness
///////////////////////////////////////////////////////////////////////////////
inline bool checkFramebufferStatus(GLenum status)
{
    switch(status)
    {
    case GL_FRAMEBUFFER_COMPLETE:
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
#ifndef USE_EGL
    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
        std::cout << "[ERROR] Framebuffer incomplete: Draw buffer." << std::endl;
        return false;

    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
        std::cout << "[ERROR] Framebuffer incomplete: Read buffer." << std::endl;
        return false;
#endif
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



/**
 * FBO for OpenGL 3.3
 *
 * NB: MRT in 3.3
   ...
   GLenum targ [3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_NONE, GL_DEPTH_ATTACHMENT };
	glDrawBuffers (4, targ);

 * Note on output and buffers:
 	   glBindFragDataLocation (renderer_1prog, GL_COLOR_ATTACHMENT0, "diffuse_out");
   ALTERNATIVE 
   to layout(location = x)
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

	enum ReadFormat { ColorRGB, Depth};

	/// raw means that 
	bool getDataRaw(std::vector<uint8_t> & data, ReadFormat f)
	{
		GLenum format = GL_RGB,type = GL_UNSIGNED_BYTE;
		int size = 1;
		switch(f)
		{
			case ColorRGB: format = GL_RGB; type = GL_UNSIGNED_BYTE; size = 3; break;
			case Depth: format = GL_DEPTH_COMPONENT; type = GL_FLOAT; size = 4; break;
		}
		data.resize(size_.width*size_.height*size);
#ifndef USE_EGL
		glBindFramebuffer(GL_READ_FRAMEBUFFER,resource_);
		glReadPixels(0,0,size_.width,size_.height,format,type,&data[0]);
		glBindFramebuffer(GL_READ_FRAMEBUFFER,0);
#else
		glBindFramebuffer(GL_FRAMEBUFFER,resource_);
		glReadPixels(0,0,size_.width,size_.height,format,type,&data[0]);
		glBindFramebuffer(GL_FRAMEBUFFER,0);
#endif
		return true;
	}

	/// raw means that 
	bool getDataRaw(std::vector<float> & data, ReadFormat f)
	{
		GLenum format = GL_RGB,type = GL_UNSIGNED_BYTE;
		int size = 1;
		switch(f)
		{
			case ColorRGB: format = GL_RGB; type = GL_FLOAT; size = 3; break;
			case Depth: format = GL_DEPTH_COMPONENT; type = GL_FLOAT; size = 1; break;
		}
		data.resize(size_.width*size_.height*size);
#ifndef USE_EGL
		glBindFramebuffer(GL_READ_FRAMEBUFFER,resource_);
		glReadPixels(0,0,size_.width,size_.height,format,type,&data[0]);
		glBindFramebuffer(GL_READ_FRAMEBUFFER,0);
#else
		glBindFramebuffer(GL_FRAMEBUFFER,resource_);
		glReadPixels(0,0,size_.width,size_.height,format,type,&data[0]);
		glBindFramebuffer(GL_FRAMEBUFFER,0);
#endif
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
		if(colors_ == 0)
			glDrawBuffer(GL_NONE);
		else
		{
			GLenum DrawBuffers[4] = {GL_COLOR_ATTACHMENT0,GL_COLOR_ATTACHMENT1,GL_COLOR_ATTACHMENT2,GL_COLOR_ATTACHMENT3};
			glDrawBuffers(colors_,DrawBuffers);
		}		
	}

	void unbind(GLenum what= GL_FRAMEBUFFER)
	{
		glBindFramebuffer(what,0);
		glDrawBuffer(GL_BACK);
	}

	/// copies the depth buffer from the current FBO to the default one
	void blitdepth()
	{
		glBindFramebuffer(GL_READ_FRAMEBUFFER, resource_);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); // Write to default framebuffer
		glBlitFramebuffer(
			  0, 0, size_.width, size_.height, 0, 0, size_.width, size_.height, GL_DEPTH_BUFFER_BIT, GL_NEAREST
		);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	GLSize size() const { return size_; }

	/**
	 * Use this scope tool for preparing the FBO. If the FBO is not initialized it is inited, and then bound
	 * At the end of the bount it is checked
	 */
	struct Setup
	{
		FBO & fbo;
		int db = 0;
		Setup(FBO & f) : fbo(f)
		{
			if(!fbo)
				fbo.init();
			fbo.bind();
		}

		~Setup()
		{
			std::cout << "FBO finish with " << db << " colors " << std::endl;
			fbo.colors_ = db;
			if(db == 0)
				glDrawBuffer(GL_NONE);
			else
			{
				GLenum DrawBuffers[4] = {GL_COLOR_ATTACHMENT0,GL_COLOR_ATTACHMENT1,GL_COLOR_ATTACHMENT2,GL_COLOR_ATTACHMENT3};
				glDrawBuffers(db,DrawBuffers);
			}
			fbo.checkvalidate();
			fbo.unbind();
		}


		/**
		 * Color Attachment from allocated texture
		 */
		void attachcolor(const Texture & t, int index = 0)
		{
			GLSize & size_ = fbo.size_;
			if(!t)
				std::cerr << "FBO attachcolor invalid texture for color attachment " << index << " @fbo " << (GLuint)fbo << std::endl;
			else
			{
				size_ = t.size();
				glERR("preattachcolor");
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, GL_TEXTURE_2D, (GLuint)t, 0);		
				std::cerr << "FBO attachcolor texture for color attachment:" << index << " " << (GLuint)t << " " << size_.width << "x" << size_.height << " @fbo " << (GLuint)fbo << std::endl;
				glERR("attachcolor");
				db++;
			}
		}

		/**
		 * Depth Attachment: ATTENTION OpenGL ES2 needs GL_DEPTHCOMPONENT16
		 */
		void attachdepth(const Texture & t)
		{
			GLSize & size_ = fbo.size_;
			if(!t)
				std::cerr << "FBO attachcolor invalid texture for depth @fbo " << (GLuint)fbo << std::endl;
			else
			{
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, (GLuint)t, 0);
				std::cerr << "FBO attachdepth texture for depth attachment:" << (GLuint)t  << " @fbo " << (GLuint)fbo << std::endl;
				glERR("attachdepth");
			}
		}


		void attach(const ColorTexture & t, int index = 0)
		{
			GLSize & size_ = fbo.size_;
			size_ = t.size();
			glERR("preattachcolor");
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, GL_TEXTURE_2D, (GLuint)t, 0);		
			std::cerr << "FBO attachcolor texture for color attachment:" << index << " texture " << (GLuint)t << " " << size_.width << "x" << size_.height << " @fbo " << (GLuint)fbo << std::endl;
			glERR("attachcolor");
			db++;
		}

		void attach(const DepthTexture & t)
		{
			GLSize & size_ = fbo.size_;
			glERR("preattachdepth");
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, (GLuint)t, 0);
			glERR("attachdepth");
			std::cerr << "FBO attachdepth texture for depth attachment texture " << (GLuint)t << " " << size_.width << "x" << size_.height << " @fbo " << (GLuint)fbo << std::endl;
		}

		void nocolor()
		{
			glDrawBuffer(GL_NONE);
			glReadBuffer(GL_NONE);		
		}

		bool makergb()
		{
			GLSize & size_ = fbo.size_;
			if(size_.width == 0)
				return false;
			GLuint & rboColor_ = fbo.rboColor_;
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
			GLSize & size_ = fbo.size_;
			if(size_.width == 0)
				return false;
			GLuint & rboDepthStencil_ = fbo.rboDepthStencil_;
			if(!rboDepthStencil_)
				glGenRenderbuffers(1, &rboDepthStencil_);
			glBindRenderbuffer(GL_RENDERBUFFER, rboDepthStencil_);
	#ifndef USE_EGL
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8,size_.width,size_.height);
			glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER,rboDepthStencil_);		
	#else
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16,size_.width,size_.height);
			glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,rboDepthStencil_);		
	#endif
			glBindRenderbuffer(GL_RENDERBUFFER, 0);
			return true;
		}



	};	
private:
	FBO(const FBO & );
	GLSize size_ = {0,0};
	GLuint resource_ = 0;
	GLuint rboDepthStencil_ = 0,rboColor_ = 0;
	int colors_ = 0;
};
}