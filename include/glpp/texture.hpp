#include "SOIL/SOIL.h"
#include "stb_image_write.h"
#include "lodepng.h"
#ifndef COCO_FATAL
#define COCO_ERR() std::cout
#define COCO_FATAL() std::cout
#define COCO_LOG(x) std::cout
#endif
namespace glpp
{

#if 0
class XTexture
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

	~XTexture()
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
	bool getDataRaw(std::vector<uint8_t> & data)
	{
		data.resize(width()*height()*itemsize_);
		glBindTexture(GL_TEXTURE_2D,resource_);
		glGetTexImage(GL_TEXTURE_2D,0,format_,type_,&data[0]);
		glBindTexture(GL_TEXTURE_2D,0);
		return true;
	}

	XTexture() {}

private:
	GLenum format_ = 0,type_ = 0;
	int itemsize_ = 4;
	bool flipped_ = false;
	GLSize size_;
	GLSize realsize_;
	GLuint resource_ = 0;

	XTexture(const XTexture & );
};



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

inline	bool XTexture::load(const char * name, bool expanded)
	{
		release();
		int width;
		int height;
		int channels;

		unsigned char * image = SOIL_load_image(name, &width, &height, &channels, 0);

		if (image == NULL) 
		{

			return false;
		}
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
		SOIL_free_image_data(image);
		
		GLint realwidth = 0;
		GLint realheight = 0;
		glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_WIDTH,&realwidth);
		glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_HEIGHT,&realheight);
		std::cout << "Texture real " << realwidth << " x " << realheight << " for "  << width << " x " << height << " channels " << channels << std::endl; 
		size_ = GLSize(width,height);
		realsize_ = GLSize(realwidth,realheight);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		float color[] = { 1.0f, 0.0f, 0.0f, 1.0f };
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, color);
		unbind();
		return true;
	}
#endif


class Texture
{
public:
    Texture()
    {}
	~Texture()
    {
        release();
    }
    void initcolor(GLSize size = {0, 0}, GLenum internalFormat = GL_RGBA,
              GLenum externalFormat = GL_RGBA,GLenum type = GL_UNSIGNED_BYTE)
    {
        release();
        size_ = size;
        format_ = internalFormat;
        type_ = type;

        glGenTextures(1, &resource_);
        //glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, resource_);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, size.width, size.height,
                     0, externalFormat, type, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        //glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);

        channels_ = format_ == GL_RED  || format_ == GL_DEPTH_COMPONENT ? 1 : format_ == GL_RGBA ? 4: 3;
    }
    void initdepth(GLSize size, bool usefloat = true)
    {
        release();
        format_ = GL_DEPTH_COMPONENT;
        channels_ = 1;
        type_ = usefloat ? GL_FLOAT: GL_UNSIGNED_SHORT;
        size_ = size;
        glGenTextures(1, &resource_);
        glBindTexture(GL_TEXTURE_2D, resource_);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, size_.width, size_.height,
                         0, GL_DEPTH_COMPONENT, type_, NULL);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D,0);
    }
    bool load(const std::string &path)
    {
        if (!valid())
        {
            glGenTextures(1, &resource_);
        }

        COCO_LOG(2) << "Loading texture file " <<  path.c_str();
        unsigned char* img = SOIL_load_image(path.c_str(), &size_.width,
                                             &size_.height, &channels_, 0);
        if (!img)
        {
            COCO_ERR() << "Failed to load texture image\n";
            return false;
        }
        //int img_lenght = size_.width * size_.height * channels_ * sizeof(u_int8_t);
        if(channels_ == 4)
            format_ = GL_RGBA;
        else if(channels_ == 1)
            format_ = GL_RED;
        if(!resource_)
            glGenTextures(1, &resource_);
        bind();
        glTexImage2D(GL_TEXTURE_2D, 0, format_, size_.width, size_.height,
                     0, format_, type_, img);
        unbind();
        SOIL_free_image_data(img);
        return true;
    }
    //void load(int img_width, int img_height);
    void load(uint8_t *image, int width, int height,
    		  GLenum source_format = GL_RGB, GLenum target_format = GL_RGB,
    		  GLenum data_type = GL_UNSIGNED_BYTE)
    {
    	if (!image)
        	return;
        size_.width = width;
        size_.height = height;
        format_ = target_format;
        type_ = data_type;
        bind();
        glTexImage2D(GL_TEXTURE_2D,     // Type of texture
                       0,                 // Pyramid level (for mip-mapping) - 0 is the top level
                       target_format,            // Internal colour format to convert to
                       width,          // Image width  i.e. 640 for Kinect in standard mode
                       height,          // Image height i.e. 480 for Kinect in standard mode
                       0,                 // Border width in pixels (can either be 1 or 0)
                       source_format, // Input image format (i.e. GL_RGB, GL_RGBA, GL_BGR etc.)
                       data_type,  // Image data type
                       image
                     );        // The actual image data itself
#ifndef USE_EGL
        glGenerateMipmap(GL_TEXTURE_2D);
#endif
        int w, h;
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);
//        std::cout << "loaded texture " << width << "x" << height << " resulting " << w << "x" << h << std::endl;
        unbind();
    }
    
    void bind(GLenum what = GL_TEXTURE_2D, int unit = 0) const
    {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(what, resource_);
    }
    void unbind(GLenum what = GL_TEXTURE_2D, int unit = 0) const
    {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(what, 0);
    }

    void release()
    {
        if(resource_)
        {
            glDeleteTextures(1, &resource_);
            resource_ = 0;
        }
    }

    operator GLuint () const { return resource_; }
    
    // basic full volume GL_R8 GL_UNSIGNED_BYTE
    bool init3d(const uint8_t * data, int xd, int yd, int zd)
    {
        std::array<int,3> big_size = {{xd, yd, zd}};
        return init3d(data, xd*yd*zd, big_size, big_size, false, false, true, true);
    }

    /// nbytes is the amount of data (only for check)
    /// size is the size of the 3D texture
    /// extent is the extent of the region to be copied at origin
    /// rgba means GL_RGBA vs GL_R
    /// float means GL_FLOAT vs GL_UNSIGNED_BYTE
    bool init3d(const uint8_t * data, int nbytes, const std::array<int,3> &size,
                const std::array<int,3>  &extent, bool rgba, bool is_float,
                bool clamp_me, bool interp);

    void resize(GLSize size, GLenum target_format, bool aspow2 = false)
    {
        size_ = size;
        if(aspow2)
        {
            size = nextpow2(size);
        }
        real_size_ = size;

        glBindTexture(GL_TEXTURE_2D, resource_);
        glTexImage2D(GL_TEXTURE_2D, 0, target_format, size.width,size.height, 0, target_format, GL_UNSIGNED_BYTE, NULL);
        glBindTexture(GL_TEXTURE_2D,0);
        format_ = target_format;
        type_ = GL_UNSIGNED_BYTE;
    }

    void update(int x, int y, int width, int height, uint8_t *data)
    {
        glTexSubImage2D(GL_TEXTURE_2D,     // Type of texture
                       0,                 // Pyramid level (for mip-mapping) - 0 is the top level
                       x,
                       y,
                       width,          // Image width  i.e. 640 for Kinect in standard mode
                       height,          // Image height i.e. 480 for Kinect in standard mode
                       format_, // Input image format (i.e. GL_RGB, GL_RGBA, GL_BGR etc.)
                       type_,  // Image data type
                       data
                     );
    }

    void update(int x, int y, int z, int width, int height, int depth, uint8_t *data)
    {
        glTexSubImage3D(GL_TEXTURE_3D,     // Type of texture
                       0,                 // Pyramid level (for mip-mapping) - 0 is the top level
                       x,
                       y,
                       z,
                       width,          // Image width  i.e. 640 for Kinect in standard mode
                       height,          // Image height i.e. 480 for Kinect in standard mode
                       depth,
                       format_, // Input image format (i.e. GL_RGB, GL_RGBA, GL_BGR etc.)
                       type_,  // Image data type
                       data
                     );
    }


    bool getDataRaw(std::vector<uint8_t> & data) const
    {
        data.resize(width() * height() * channels_ * (type_ == GL_UNSIGNED_BYTE?1:type_==GL_UNSIGNED_SHORT?2:4));
        glBindTexture(GL_TEXTURE_2D, resource_);
        glGetTexImage(GL_TEXTURE_2D, 0, format_, type_, &data[0]);
        glBindTexture(GL_TEXTURE_2D, 0);
        return true;
    }

    bool save(const std::string &filename) const
    {
        std::vector<uint8_t> buffer;
        getDataRaw(buffer);
        switch(type_)
        {
            case GL_UNSIGNED_SHORT:
            {
               // encodeOneStep(filename,buffer,, unsigned width, unsigned height)
               if(channels_ == 1)
               {
                lodepng_encode_file(filename.c_str(),&buffer[0],size_.width,size_.height,LCT_GREY,16);
                return true;
            }
            else
            return false;                
            }
            case GL_FLOAT:
            {
            return false;

            }
            case GL_UNSIGNED_BYTE:
            {
                stbi_write_png(filename.c_str(),size_.width, size_.height,channels_,&buffer[0],size_.width*channels_);
            return true;
            }
            default:
            return false;
        }
    }

    int width() const { return size_.width; }
    int height() const { return size_.height; }
    GLSize realsize() const { return size_; }
    GLSize size() const { return size_; }
    bool valid() const { return resource_ != 0; }
    GLuint resource() const { return resource_; }

    bool flipped() const {return false;}
private:
    Texture(const Texture &);

    GLuint resource_ = 0;
    GLSize size_; // stored size
    GLSize real_size_; // original size
    GLenum format_ = GL_RGB; // GL_RGB GL_RED GL_RGBA
    GLenum type_ = GL_UNSIGNED_BYTE; // GL_FLOAT GL_UNSIGNED_SHORT GL_UNSIGNED_BYTE
    int channels_ = 3; // channels for format
    bool flipped_ = false; // stored flipped
};

// types
class DepthTexture: public Texture
{
public:
    void init(GLSize size, bool asfloat) { Texture::initdepth(size,asfloat); }
};

// types
class ColorTexture: public Texture
{
public:
    void init(GLSize size, bool alpha) { Texture::initcolor(size,alpha ? GL_RGBA: GL_RGB,GL_UNSIGNED_BYTE); }
};

/// Texture sampler overrides Texture parameters attached to the same Texture unit
class Sampler
{
public:
    Sampler(): resource_(0) {  }

    void init()
    {
        release();
        glGenSamplers (1, &resource_);       
    }

    // setter
    void seti(GLenum param, int value)
    {
        glSamplerParameteri(resource_,param,value);
    }

    void bind(int unit)
    {
        glBindSampler(unit,resource_);
    }

    void unbind(int unit)
    {
        glBindSampler(unit,0);
    }

    void release()
    {
        if(resource_)
        {
            glDeleteSamplers(1,&resource_);
        }
    }

    operator GLuint ()
    {
        return resource_;
    }

    ~Sampler()
    {
        release();
    }
private:
    Sampler(const Sampler&);
    GLuint resource_;
};


}