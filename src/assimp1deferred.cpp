// See: http://aras-p.info/texts/CompactNormalStorage.html#method00xyz
// http://learnopengl.com/#!Advanced-Lighting/Deferred-Shading
// http://www.ogldev.org/www/tutorial36/tutorial36.html
// http://gamedevs.org/uploads/deferred-shading-tutorial.pdf
#include "glpp/app.hpp"
#include "glpp/draw.hpp"
#include "glpp/gleigen.hpp"
#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags
#include <Eigen/Geometry>
#include <iostream>
#include "assimpex.hpp"
#include "glpp/ArcBall.hpp"

struct Light {
    Eigen::Vector3f position;
    Eigen::Vector3f color;
    
    float linear;
    float quadratic;
};


using namespace glpp;

	const char * meshv = GLSL330(
		layout(location = 0) in vec3 vertexPosition_modelspace;
		layout(location = 1) in vec3 vertexNormal_modelspace;
		layout(location = 2) in vec3 vertexTexCoords;
		uniform mat4 m_VM;
		uniform mat4 m_M;
		uniform mat4 m_P;
		uniform mat3 m_N;
		uniform vec3 l_pos;
		out vec3 uv;
		out vec3 DataIn_normal;
		out vec3 DataIn_lightDir;
		out vec3 DataIn_eye;
		out vec3 FragPos;

		void main(){
			vec4 worldPos = m_M * vec4(vertexPosition_modelspace, 1.0f);
	    	FragPos = worldPos.xyz; 

			vec4 pos = m_VM *  vec4(vertexPosition_modelspace, 1.0f);
	    	DataIn_normal = normalize(m_N * vertexNormal_modelspace);
	    	DataIn_lightDir = vec3(l_pos - pos.xyz);
	    	DataIn_eye = vec3(-pos);
	    	uv = vertexTexCoords;

		    gl_Position =  m_P*pos;
		}
	);

const char * meshf = GLSL330(
	layout (location = 0) out vec4 gAlbedoSpec;
	layout (location = 1) out vec3 gNormal;
	layout (location = 2) out vec3 gPosition;

		uniform vec4 diffuse;
		uniform vec4 ambient;
		uniform vec4 specular;
		uniform sampler2D tex;
		uniform float shininess;

	in vec3 uv;
	in vec3 DataIn_normal;
	in vec3 DataIn_lightDir;
	in vec3 DataIn_eye;
	in vec3 FragPos;

	//uniform sampler2D texture_diffuse1;
	//uniform sampler2D texture_specular1;

	void main()
	{    
		vec4 spec = vec4(0.0);

		vec3 n = normalize(DataIn_normal);
		vec4 o = texture(tex,uv.xy);
		//o += diffuse;
		//o += vec4(uv,1.0);
		//color = max(intensity * o + spec, ambient);
		//color = o+ 0.00001*vec4(uv,1.0) + 0.00001*diffuse;

	    // Store the fragment position vector in the first gbuffer texture
	    gPosition = FragPos;
	    // Also store the per-fragment normals into the gbuffer
	    gNormal = normalize(DataIn_normal);
	    // And the diffuse per-fragment color
	    //gAlbedoSpec.rgb = texture(texture_diffuse1, TexCoords).rgb;
	    // Store specular intensity in gAlbedoSpec's alpha component
	    //gAlbedoSpec.a = texture(texture_specular1, TexCoords).r;

	    gAlbedoSpec = o; //vec4(normalize(FragPos),1.0);
	}

);

const char * defshav = GLSL330(
layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texCoords;

out vec2 TexCoords;

void main()
{
    gl_Position = vec4(position, 1.0f);
    TexCoords = texCoords;
}

);

const char * defshaf = GLSL330(

out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;
uniform vec3 viewPos;


struct Light {
    vec3 Position;
    vec3 Color;
    
    float Linear;
    float Quadratic;
};
const int NR_LIGHTS = 1;
uniform Light lights[NR_LIGHTS];


void main()
{             
    vec3 FragPos = texture(gPosition,   TexCoords).rgb;
    vec3 Normal  = texture(gNormal,     TexCoords).rgb; // TODO: optimize decode
    vec4 DiSpec  = texture(gAlbedoSpec, TexCoords);
    vec3 Diffuse = DiSpec.rgb;
    float Specular = DiSpec.a;
    
    vec3 lighting  = Diffuse * 0.8; // hard-coded ambient component
    vec3 viewDir  = normalize(viewPos - FragPos);

    // Then calculate lighting as usual

    // Diffuse
    vec3 lightDir = normalize(lights[0].Position - FragPos);
    vec3 diffuse = max(dot(Normal, lightDir), 0.0) * Diffuse * lights[0].Color;
    // Specular
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    float spec = pow(max(dot(Normal, halfwayDir), 0.0), 16.0);
    vec3 specular = lights[0].Color * spec * Specular;
    // Attenuation
    float distance = length(lights[0].Position - FragPos);
    float attenuation = 1.0 / (1.0 + lights[0].Linear * distance + lights[0].Quadratic * distance * distance);
    diffuse *= attenuation;
    specular *= attenuation;
    lighting += diffuse + specular;
    FragColor = vec4(Normal, 1.0);

}
);

#if 0
    for(int i = 0; i < NR_LIGHTS; ++i)
    {
        // Diffuse
        vec3 lightDir = normalize(lights[i].Position - FragPos);
        vec3 diffuse = max(dot(Normal, lightDir), 0.0) * Diffuse * lights[i].Color;
        // Specular
        vec3 halfwayDir = normalize(lightDir + viewDir);  
        float spec = pow(max(dot(Normal, halfwayDir), 0.0), 16.0);
        vec3 specular = lights[i].Color * spec * Specular;
        // Attenuation
        float distance = length(lights[i].Position - FragPos);
        float attenuation = 1.0 / (1.0 + lights[i].Linear * distance + lights[i].Quadratic * distance * distance);
        diffuse *= attenuation;
        specular *= attenuation;
        lighting += diffuse + specular;
    }    
   #endif




struct Deferred
{
	VAO vao;
	VBO<1> vvbo;
	FBO fbo;
	GLSize size_;
	ColorTexture trgb;
	ColorTexture tnormal;
	ColorTexture tpos;
	Shader sha;
	WrappedUniform<Eigen::Vector3f> viewPos;

	Deferred(int width,int height): size_(width,height)
	{
		vao.init();
		vvbo.init();

		// prepare textues
		trgb.init(size_,true,false); 
		tnormal.init(size_,true,false);
		tpos.init(size_,true,false); 
		{
			GLScope<Texture> s(tpos);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			int w, h,f;
			int miplevel = 0;
			glGetTexLevelParameteriv(GL_TEXTURE_2D, miplevel, GL_TEXTURE_WIDTH, &w);
			glGetTexLevelParameteriv(GL_TEXTURE_2D, miplevel, GL_TEXTURE_HEIGHT, &h);
			glGetTexLevelParameteriv(GL_TEXTURE_2D, miplevel, GL_TEXTURE_INTERNAL_FORMAT, &f);
			std::cout << "pos texture " << w << " " << h << " " << std::hex << f << std::dec << std::endl;
		}
		{
			GLScope<Texture> s(tnormal);
			int w, h,f;
			int miplevel = 0;
			glGetTexLevelParameteriv(GL_TEXTURE_2D, miplevel, GL_TEXTURE_WIDTH, &w);
			glGetTexLevelParameteriv(GL_TEXTURE_2D, miplevel, GL_TEXTURE_HEIGHT, &h);
			glGetTexLevelParameteriv(GL_TEXTURE_2D, miplevel, GL_TEXTURE_INTERNAL_FORMAT, &f);
			std::cout << "normal texture " << w << " " << h << " " << std::hex << f << std::dec << std::endl;
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		}
		{
			FBO::Setup s(fbo);
			s.attach(tnormal,0); //  first is normal
			s.attach(trgb,1);
			s.attach(tpos,2);
			s.makedepth();
		}

		{
		    if(!sha.load(defshav, defshaf, 0, 0, 0, false))
		    {
		    	std::cout << "failed Deferred shader" << std::endl;
		    	exit(-1);
		    }
		    // link the input uniforms for the textures FOR OUTPUT
		    GLScope<Shader> ss(sha);
		    sha.uniform<int>("gAlbedoSpec") << 1;
		    sha.uniform<int>("gPosition") << 2;
		    sha.uniform<int>("gNormal") << 0;
		}

		viewPos = sha.uniform<Eigen::Vector3f>("viewPos");

        {
        	const int pos_attrib = 0;
        	const int tex_attrib = 1;
			const int vertex_size = 4 * sizeof(float);
			const int texture_offset = 2 * sizeof(float);
	        float vertices[] = {
	        -1.0f,  1.0f, 0.0f, 1.0, // Left Top 
	         -1.0f, -1.0f, 0,0.0, // Left Bottom
	         1.0f, 1.0f, 1.0,1.0, // Right Top
	        1.0f, -1.0f, 1.0,0.0,  // Bottom-left
	        };

	        GLScope<VAO> xvao(vao);
            GLScope<VBO<1>> xvbo(vvbo, GL_ARRAY_BUFFER);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
            glEnableVertexAttribArray(pos_attrib);
            glEnableVertexAttribArray(tex_attrib);
            glVertexAttribPointer(pos_attrib, 2, GL_FLOAT, GL_FALSE,vertex_size, 0);
            glVertexAttribPointer(tex_attrib, 2, GL_FLOAT, GL_FALSE,vertex_size, (void*)texture_offset);
        }
//            GLScope<VBO<1>> xvbo(tvbo, GL_ELEMENT_ARRAY_BUFFER);
//            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements, GL_STATIC_DRAW);
	}

	void setLight(int index, Light l)
	{
		GLScope<Shader> x(sha);
		std::string prefix = "lights[" + std::to_string(index) + "].";

		sha.uniform<Eigen::Vector3f>(prefix + "position") << l.position;
		sha.uniform<Eigen::Vector3f>(prefix + "color") << l.color;
		sha.uniform<float>(prefix + "linear") << l.linear;
		sha.uniform<float>(prefix + "quadratic") << l.quadratic;
	}

	void render(Eigen::Matrix4f viewcam)
	{

        GLScope<VAO> xvao(vao);
        GLScope<Shader> xsha(sha);        
        GLScope<Texture> t1(trgb,   GL_TEXTURE_2D,1);
        GLScope<Texture> t2(tpos,   GL_TEXTURE_2D,2);
        GLScope<Texture> t3(tnormal,GL_TEXTURE_2D,0);
        viewPos << viewcam.block<3,1>(0,3);
        GLScopeDisable<GL_DEPTH_WRITEMASK> xdw;
        GLScopeDisable<GL_DEPTH_TEST> xdt; // no need to write or read depth
        //glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        //GLScope<VBO<1>> xvbo(tvbo_, GL_ELEMENT_ARRAY_BUFFER);
        //glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}

};

int main(int argc, char **argv)
{
	if(argc  < 2)
	{
		std::cerr << "needed filename of assimp resource\n";
		return -1;
	}
	int width = 640;
	int height = 480;
	auto window = glpp::init(width,height,"hello deferred");
	Deferred def(window->realWidth,window->realHeight);
	glERR("opengl:after deferred");

	std::vector<std::unique_ptr<basicobj> >  objects;
	std::shared_ptr<material> mat = std::make_shared<material>();
	assimploader(argv[1],objects,mat);


	{
	    if(!mat->sha.load(meshv, meshf, 0, 0, 0, false))
	    	exit(-1);
	    GLScope<Shader> ss(mat->sha);
	    mat->initshader();
		glERR("opengl:setup");
	}
	std::cout << "go...\n";
	glEnable(GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	Light l;
	l.position = Eigen::Vector3f(1.0,1.0,2.0);
	l.color = Eigen::Vector3f(1.0,1.0,0.0);
	l.linear = 0.0;
	l.quadratic = 0.0;
	def.setLight(0,l);

	//TODO ArcBall ab(glm::vec3(0,0,0),0.75,);
	auto Proj = glpp::eigen::perspective<float>(60.0f,         // The horizontal Field of View, in degrees : the amount of "zoom". Think "camera lens". Usually between 90° (extra wide) and 30° (quite zoomed in)
	    width/(float)height, // Aspect Ratio. Depends on the size of your window. Notice that 4/3 == 800/600 == 1280/960, sounds familiar ?
	    0.1f,        // Near clipping plane. Keep as big as possible, or you'll get precision issues.
	    300.0f       // Far clipping plane. Keep as little as possible.
	);
	auto View      = glpp::eigen::lookAt<float>({0,2,2},{0,0,0},{0,1,0});
	Eigen::Matrix4f Model = Eigen::Matrix4f::Identity();
	Model.block<3,3>(0,0) = Eigen::Matrix3f::Identity();
	std::cout << "Proj is\n" << Proj  << std::endl;
	std::cout << "View is\n" <<  View  << std::endl;
	std::cout << "Model is\n" <<  Model << std::endl;
	std::cout << "Matrix is\n" << Proj * View * Model << std::endl;

	ArcBall hb(Eigen::Vector3f(0,0,0),0.75,window->screenToNDC());

	matrixsetup ms;
	ms.V = View;
	ms.M = Model;
	ms.P = Proj;
	ms.N = (ms.V*ms.M).block<3,3>(0,0).transpose();

	window->movefx = [&hb] (GLFWwindow *w,double x, double y) 
	{
		if(glfwGetMouseButton(w,0) == GLFW_PRESS)
		{
			hb.drag(Eigen::Vector2f(x,y));
		}
	};

	window->mousefx = [&hb] (GLFWwindow *w,int button, int action, int mods) 
	{
		if(button == 0 && action == GLFW_PRESS)
		{
			double p[2];
			glfwGetCursorPos(w,&p[0],&p[1]); // make helper
			hb.beginDrag(Eigen::Vector2f(p[0],p[1]));
		}		
	};	
	do {

		glERR("opengl:pre defrender");
		// render to multi target FBO owned by the deferred tool
		{
			GLScope<FBO> s(def.fbo);
	        glClearColor(0.0,0.0,0.0,1.0);
			glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
			ms.M = hb.getTransformation();
			for(auto & o : objects)
				o->render(ms);
		}

		glERR("opengl:pre defrender");
		def.render(ms.V);
		glERR("opengl:after def render");

		glfwSwapBuffers(*window);
		glfwPollEvents();
	} 
	while( glfwGetKey(*window, GLFW_KEY_ESCAPE ) != GLFW_PRESS && glfwWindowShouldClose(*window) == 0 );
	
	if(argc > 2)
	{
		if(!def.trgb.save("color.png"))
			std::cout << "failed saveccolor\n";
		if(!def.tnormal.save("normal.exr"))
			std::cout << "failed save normal\n";
		if(!def.tpos.save("pos.exr"))
			std::cout << "failed save tpos\n";
	}
	glfwTerminate();

}