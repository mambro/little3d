#include "glpp/app.hpp"
#include "glpp/draw.hpp"
#include <iostream>

using namespace glpp;
#define COCO_ERR() std::cout

const char* vertex_shader_base = GLSL330(
layout(location = 0) in vec2 position;
layout(location = 1) in vec2 texcoord;
out vec2 texture_coord;
void main() {
    gl_Position =  vec4(position, 0.0, 1.0);
    texture_coord = texcoord.xy;
}
);

const char* fragment_shader_base = GLSL330(
in vec2 texture_coord;
out vec4 color;
uniform bool flip;
uniform sampler2D texture_uniform;
void main() {
    vec2 tt = vec2(texture_coord.x, flip ? 1.0-texture_coord.y : texture_coord.y);
   color = vec4(texture(texture_uniform, tt).rgb,1.0);
}
);

/**
 * Image Processor associated to a given Image Size and Fragment
 */
class GLImageProc
{
public:
	/**
	 * Deals with given width and height
	 * Inputreduced means that the input image is placed inside a pow2 texture and processing should be limited to that
	 * Outputreduced means that the output image should be placed inside a pow2 texture. This allows to expand the texture
	 */
    void init(const char * vert = nullptr, const char * frag = nullptr)
    {
        if (vert && frag)
            shader_.load(vert, frag,"");
        else
            shader_.load(vertex_shader_base, fragment_shader_base, "",0,0,false);

        GLint pos_attrib, tex_attrib;
        {
            GLScope<Shader> ss(shader_);
            pos_attrib = shader_.attributeLocation("position");
            tex_attrib = shader_.attributeLocation("texcoord");
            flipper_ = shader_.uniformLocation("flip");
            int texture_location = shader_.uniformLocation("texture_uniform");
            if(texture_location < 0 || tex_attrib < 0 || pos_attrib < 0)
            {
                COCO_ERR() << "ImageProcessing shader misses: tex uniform,\
                               texcoord or position attributes\n";
            }
            shader_.uniform<int>(texture_location) << 0;
        }

        float wa = 1.0;
        float ha = 1.0;
        float vertices[] = {
        //  Position      Color             Texcoords
        -1.0f,  1.0f, 0.0f, ha, // Top-left
         1.0f,  1.0f, wa,   ha, // Top-right
         1.0f, -1.0f, wa,   1.0f - ha, // Bottom-right
        -1.0f, -1.0f, 0.0f, 1.0f - ha  // Bottom-left
        };

        const int vertex_size = 4 * sizeof(float);
        const int texture_offset = 2 * sizeof(float);
        GLuint elements[] = {0, 1, 2, 2, 3, 0};

        vbo_.init();
        vao_.init();

        GLScope<VAO> xvao(vao_);
        {
            GLScope<VBO<2>> xvbo(vbo_, GL_ARRAY_BUFFER, 0);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
            glEnableVertexAttribArray(pos_attrib);
            glVertexAttribPointer(pos_attrib, 2, GL_FLOAT, GL_FALSE,vertex_size, 0);
            glEnableVertexAttribArray(tex_attrib);
            glVertexAttribPointer(tex_attrib, 2, GL_FLOAT, GL_FALSE,vertex_size, (void*)texture_offset);
        }
        {
            GLScope<VBO<2>> xvbo(vbo_, GL_ELEMENT_ARRAY_BUFFER, 1);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements, GL_STATIC_DRAW);
        }
    }

    void initFBO(int width, int height, bool input_reduced, bool output_reduced)
    {
        int width2 = pow2roundup(width);
        int height2 = pow2roundup(height);
        // float wa = input_reduced ? width / (float)width2 : 1.0;
        // float ha = input_reduced ? height / (float)height2 : 1.0;
        if(output_reduced)
            texture_.init(GLSize(width2, height2));
        else
            texture_.init(GLSize(width, height));


        fbo_.init();
        {
            GLScope<FBO> xfbo(fbo_);
            fbo_.attach(texture_);
            checkFramebufferStatus(fbo_.validate());
        }
    }

	/// returns the texture that is computed by run
    Texture & getOutput() { return texture_; }

    void getImage(std::vector<uint8_t> &img, FBO::ReadFormat format = FBO::ColorRGB)
    {
        fbo_.getDataRaw(img, format);
    }

	/// runs the algorithm
    Texture & runOnScreen(Texture & input)
    {
        {
            GLScope<Shader> ss(shader_);
            shader_.uniform<int>("flip") << 1;
        }
        renderStep(input);
        return texture_;
    }

    Texture & runOnFbo(Texture & input, bool flip = false)
    {
        GLScope<FBO> xfbo(fbo_);
        GLViewportScope view(0,0,fbo_.size().width,fbo_.size().height);
        {
            GLScope<Shader> ss(shader_);
            shader_.uniform<int>("flip") << flip;
        }
        glClearColor(0,0.0,0.0,0.0);
        glClear(GL_COLOR_BUFFER_BIT);

        renderStep(input);
        return texture_;
    }

    void renderStep(Texture & input)
    {
        glDepthMask(GL_FALSE);
        GLScope<Texture> xtex(input);
        GLScope<VAO> xvao(vao_);
        GLScope<Shader> xsha(shader_);
        GLScope<VBO<2>> xvbo(vbo_, GL_ELEMENT_ARRAY_BUFFER, 1);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glDepthMask(GL_TRUE);
    }

private:
    GLint flipper_;
    FBO fbo_; /// support FBO
    Shader shader_; /// support Shader object
    Texture texture_; /// output texture attached to the FBO
    VBO<2> vbo_; /// two VBOs used for the coordinates
    VAO  vao_; /// the VAO used for the points
};

int main(int argc, char **argv)
{
	int width = 640;
	int height = 480;
	auto window = glpp::init(width,height);
	glERR("opengl:after init app");

	GLImageProc img;
	Texture tex;
	tex.init();
	if(!tex.load(argv[1]))
		return 0;
	img.init();
	std::cout << "loaded " << tex.size() << " " << tex.realsize() << "  " << tex.flipped() << " res:" << (int)tex << std::endl;
		glERR("opengl:load");

		glERR("opengl:initimg");
		img.runOnScreen(tex);
		glERR("opengl:render");

	do {
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		img.runOnScreen(tex);
		glfwSwapBuffers(window);
		glfwPollEvents();
	} 
	while( glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS && glfwWindowShouldClose(window) == 0 );
	glfwTerminate();
}