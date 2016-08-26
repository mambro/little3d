#pragma once
#include <array>
#include "little3d/fbo.hpp"

namespace little3d
{
    /**
     * Image Processor associated to a given Image Size and Fragment
     */
    class GLImageProc
    {
    public:
        GLImageProc() { init(); }

        GLImageProc(const char * vert, const char * frag) { init(vert,frag);}
    	/**
    	 * Deals with given width and height
    	 * Inputreduced means that the input image is placed inside a pow2 texture and processing should be limited to that
    	 * Outputreduced means that the output image should be placed inside a pow2 texture. This allows to expand the texture
    	 */
        void init(const char * vert = nullptr, const char * frag = nullptr)
        {

            static const char* vertex_shader_base = GLSL330(
            layout(location = 0) in vec2 position;
            layout(location = 1) in vec2 texcoord;
            out vec2 texture_coord;
            void main() {
                gl_Position =  vec4(position, 0.0, 1.0);
                texture_coord = texcoord.xy;
            }
            );

            static const char* fragment_shader_base = GLSL330(
            in vec2 texture_coord;
            out vec4 color;
            uniform bool flip;
            uniform sampler2D texture_uniform;
            void main() {
                vec2 tt = vec2(texture_coord.x, flip ? 1.0-texture_coord.y : texture_coord.y);
               color = vec4(texture(texture_uniform, tt).rgb,1.0);
               //color = vec4(texture_coord.xy,0,1.0);
            }
            );


            if (vert && frag)
                shader_.load(vert, frag,"");
            else
                shader_.load(vertex_shader_base, fragment_shader_base, "",0,0,false);

            //GLint pos_attrib, tex_attrib;
            {
                GLScope<Shader> ss(shader_);
                //pos_attrib = shader_.attributeLocation("position");
                //tex_attrib = shader_.attributeLocation("texcoord");
                flipper_ = shader_.uniformLocation("flip");
                int texture_location = shader_.uniformLocation("texture_uniform");
                if(texture_location < 0/* || tex_attrib < 0 || pos_attrib < 0)*/)
                {
                    COCO_ERR() << "ImageProcessing shader misses: tex uniform,\
                                   texcoord or position attributes\n";
                }
                else
                    shader_.uniform<int>(texture_location) << 0;
            }

            float wa = 1.0;
            float ha = 1.0;
            float vertices[] = {
            -1.0f,  1.0f,    0.0f, ha, // Left Top 
             -1.0f, -1.0f,   0,1.0f-ha, // Left Bottom
             1.0f, 1.0f,     wa,   ha, // Right Top
            1.0f, -1.0f,     wa,1.0f-ha,  // Right Bottom
            };
            std::copy(vertices,vertices+16,all_.begin());

    /*
        -1.0f,  1.0f,
        -1.0f, -1.0f,
         1.0f,  1.0f,
         1.0f, -1.0f
         */
            const int vertex_size = 4 * sizeof(float);
            const int texture_offset = 2 * sizeof(float);
            GLuint elements[] = {0, 1, 2, 2, 3, 0};

            tvbo_.init();
            vvbo_.init();
            vao_.init();

            GLScope<VAO> xvao(vao_);
            {
                GLScope<VBO<1>> xvbo(vvbo_, GL_ARRAY_BUFFER);
                glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
                glEnableVertexAttribArray(0);
                glEnableVertexAttribArray(1);
                glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,vertex_size, 0);
                glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,vertex_size, (void*)texture_offset);
            }
            {
                GLScope<VBO<1>> xvbo(tvbo_, GL_ELEMENT_ARRAY_BUFFER);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements, GL_STATIC_DRAW);
            }
        }

        /// set vertices in clip space 2D: LT LB RT RB
        void setVerticesClipSpace(const std::array<float,4*2> &vertices)
        {
            for(int i = 0; i < 4; i++)
            {
                all_[i*4] = vertices[i*2];
                all_[i*4+1] = vertices[i*2+1];
            }
            GLScope<VBO<1>> xvbo(vvbo_, GL_ARRAY_BUFFER);
            glBufferData(GL_ARRAY_BUFFER, sizeof(float)*all_.size(), all_.data(), GL_STATIC_DRAW);
        }

        void initFBO(int width, int height, bool input_reduced, bool output_reduced)
        {
            int width2 = pow2roundup(width);
            int height2 = pow2roundup(height);
            // float wa = input_reduced ? width / (float)width2 : 1.0;
            // float ha = input_reduced ? height / (float)height2 : 1.0;
            if(output_reduced)
                texture_.initcolor(GLSize(width2, height2));
            else
                texture_.initcolor(GLSize(width, height));

            {
                FBO::Setup s(fbo_);
                s.attachcolor(texture_);
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
            renderStep(input,true);
            return texture_;
        }

        Texture & runOnFbo(Texture & input, bool flip = false)
        {
            GLScope<FBO> xfbo(fbo_); // already does viewport
            glClearColor(0,0.0,0.0,0.0);
            glClear(GL_COLOR_BUFFER_BIT);
            renderStep(input,flip);
            return texture_;
        }

        void renderStep(Texture & input, bool flip)
        {
            GLScopeDisable<GL_DEPTH_WRITEMASK> xdepth;
            GLScopeDisable<GL_DEPTH_TEST> xdeepth;
            GLScope<Texture> xtex(input);
            GLScope<VAO> xvao(vao_);
            GLScope<Shader> xsha(shader_);
            shader_.uniform<int>("flip") << (flip ?1:0);
            glERR("opengl:predraw");
            //GLScope<VBO<1>> xvbo(tvbo_, GL_ELEMENT_ARRAY_BUFFER);
            //glDrawElements(GL_TRIANGLE_STRIP, 6, GL_UNSIGNED_INT, 0);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            glERR("opengl:postdraw");
        }

    private:
        std::array<float,4*4> all_;
        GLint flipper_;
        FBO fbo_; /// support FBO
        Shader shader_; /// support Shader object
        Texture texture_; /// output texture attached to the FBO
        VBO<1> tvbo_; /// two VBOs used for the coordinates
        VBO<1> vvbo_; /// two VBOs used for the coordinates
        VAO  vao_; /// the VAO used for the points
    };
}