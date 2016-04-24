// See: http://aras-p.info/texts/CompactNormalStorage.html#method00xyz
// http://learnopengl.com/#!Advanced-Lighting/Deferred-Shading
// http://www.ogldev.org/www/tutorial36/tutorial36.html
#include "glpp/app.hpp"
#include "glpp/draw.hpp"
#include "glpp/gleigen.hpp"
#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags
#include <Eigen/Geometry>
#include <iostream>

using namespace glpp;

const char * meshv = GLSL330(
	layout (location = 0) in vec3 position;
	layout (location = 1) in vec3 normal;

	out vec3 FragPos;
	out vec3 Normal;

	uniform mat4 m_M;
	uniform mat4 m_N;
	uniform mat4 m_V;
	uniform mat4 m_P;

	void main()
	{
	    vec4 worldPos = m_M * vec4(position, 1.0f);
	    FragPos = worldPos.xyz; 
	    gl_Position = (m_P*m_V) * worldPos;
	    
	    //mat3 normalMatrix = transpose(inverse(mat3(model)));
	    Normal = m_N * normal;
	}

);

const char * meshf = GLSL330(
	layout (location = 0) out vec4 gAlbedoSpec;
	layout (location = 1) out vec3 gNormal;
	layout (location = 2) out vec3 gPosition;

	in vec2 TexCoords;
	in vec3 FragPos;
	in vec3 Normal;

	//uniform sampler2D texture_diffuse1;
	//uniform sampler2D texture_specular1;

	void main()
	{    
	    // Store the fragment position vector in the first gbuffer texture
	    gPosition = FragPos;
	    // Also store the per-fragment normals into the gbuffer
	    gNormal = normalize(Normal);
	    // And the diffuse per-fragment color
	    gAlbedoSpec.rgb = texture(texture_diffuse1, TexCoords).rgb;
	    // Store specular intensity in gAlbedoSpec's alpha component
	    gAlbedoSpec.a = texture(texture_specular1, TexCoords).r;
	}

);

const char * meshvpost = GLSL330(
layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texCoords;

out vec2 TexCoords;

void main()
{
    gl_Position = vec4(position, 1.0f);
    TexCoords = texCoords;
}

);

/*
struct Light {
    vec3 Position;
    vec3 Color;
    
    float Linear;
    float Quadratic;
};
const int NR_LIGHTS = 32;
uniform Light lights[NR_LIGHTS];
*/
const char * meshfpost = GLSL330(
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;
uniform vec3 viewPos;

void main()
{             
    // Retrieve data from gbuffer
    vec3 FragPos = texture(gPosition, TexCoords).rgb;
    vec3 Normal = texture(gNormal, TexCoords).rgb; // TODO: optimize decode
    vec3 Diffuse = texture(gAlbedoSpec, TexCoords).rgb;
    float Specular = texture(gAlbedoSpec, TexCoords).a;
    
    // Then calculate lighting as usual
    vec3 lighting  = Diffuse * 0.1; // hard-coded ambient component
    vec3 viewDir  = normalize(viewPos - FragPos);

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


    FragColor = vec4(lighting, 1.0);
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



struct matrixsetup
{
	Eigen::Matrix4f V;
	Eigen::Matrix4f M;
	Eigen::Matrix4f P;
	Eigen::Matrix3f N;
};

struct material
{
	void initshader()
	{
		uV.init(sha,"m_V");
		uM.init(sha,"m_M");
		uP.init(sha,"m_P");
		uN.init(sha,"m_N");
		l_pos.init(sha,"l_pos");
		diffuse.init(sha,"diffuse");
		ambient.init(sha,"ambient");
		specular.init(sha,"specular");
		shininess.init(sha,"shininess");

		l_pos << Eigen::Vector3f(0.0,2.0,3.0);
		diffuse << Eigen::Vector4f(0.0,1.0,0.0,0.5);
		ambient << Eigen::Vector4f(0.2,0.2,0.2,0.4);
		specular << Eigen::Vector4f(0.2,0.2,0.2,0.4);
		shininess << 10;
	} 

	void update(matrixsetup & x)
	{
		uM << x.M;
		uV << x.V;
		uP << x.P;
		uN << x.N;
	}

	Shader sha;

	WrappedUniform<Eigen::Matrix4f> uM;
	WrappedUniform<Eigen::Matrix4f> uV;
	WrappedUniform<Eigen::Matrix4f> uP;
	WrappedUniform<Eigen::Matrix3f> uN;
	WrappedUniform<Eigen::Vector3f> l_pos; // light position
	WrappedUniform<Eigen::Vector4f> diffuse,ambient,specular;
	WrappedUniform<float> shininess;
};


struct basicobj
{
	basicobj()
	{
		vvbo.init();
		nvbo.init();
		tvbo.init();
		vao.init();		
	}


	void render(matrixsetup & mvp)
	{
		GLScope<VAO> v(vao);
		GLScope<VBO<1> > t(tvbo,GL_ELEMENT_ARRAY_BUFFER);
		GLScope<Shader> s(mat->sha);
		mat->update(mvp);
    	glDrawElements(GL_TRIANGLES,ntri,GL_UNSIGNED_INT,0); 
	}

	
	int ntri = 0;
	VBO<1> vvbo;
	VBO<1> nvbo;
	VBO<1> tvbo;
	VAO    vao;
	std::shared_ptr<material> mat;
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
	auto window = glpp::init(width,height,"hello",false);
		glERR("opengl:after init");
	ColorTexture trgb;
	DepthTexture tdepth;
	ColorTexture tnormal;
	ColorTexture tpos;
	tdepth.init(GLSize(width,height),false); // 16bit
	trgb.initcolor(GLSize(width,height),GL_RGBA,GL_RGBA,GL_UNSIGNED_BYTES); 
	tnormal.init(GLSize(width,height),GL_RGB16F,GL_RGB,GL_FLOAT);  // float
	tpos.init(GLSize(width,height),GL_RGB16F,GL_RGB,GL_FLOAT); // float

	FBO fbo;
	{
		FBO::Setup s(fbo);
		fbo.attach(trgb);
		fbo.attach(tnormal,1);
		fbo.attach(tpos,2);
		fbo.attach(tdepth);
		// only GLES3+ GL3.3+
	}

	std::vector<std::unique_ptr<basicobj> >  objects;

	Assimp::Importer importer;
	importer.SetPropertyInteger(AI_CONFIG_PP_PTV_NORMALIZE, 1);
	const aiScene* scene = importer.ReadFile( argv[1],aiProcess_Triangulate
		|aiProcess_GenNormals|aiProcess_PreTransformVertices); 
	    //aiProcess_CalcTangentSpace       | 
	    //aiProcess_Triangulate            |
	    //aiProcess_JoinIdenticalVertices  |
	    //aiProcess_SortByPType);
	// If the import failed, report it
	if( !scene)
		return -1;

	std::shared_ptr<material> mat = std::make_shared<material>();
	std::cout << "only one got:" << scene->mNumMeshes << " meshes" << std::endl;
	for(int i = 0; i < scene->mNumMeshes; i++)
	{
		aiMesh * m = scene->mMeshes[i];
		aiMaterial * ma = scene->mMaterials[m->mMaterialIndex];
		aiString name;
		//ma->GetTexture(aiTextureType_DIFFUSE,0,&name);
		std::cout << "Properties faces/vertices:" << m->mNumFaces << " " << m->mNumVertices << " uv:" << m->mNumUVComponents[0] << " material:" <<    m->mMaterialIndex << " texture " <<  name.C_Str() << std::endl;

		// TODO rescale
		glERR("opengl:before new");
		std::unique_ptr<basicobj> op(new basicobj());
		basicobj & o = *op;
		o.mat = mat;
		{
	std::cout << "vbo... " << std::endl;
		{
			GLScope<VBO<1> > v(o.vvbo);
			glBufferData(
	                    GL_ARRAY_BUFFER,
	                    m->mNumVertices * 3 * sizeof(float),
	                    (float *)&m->mVertices[0],
	                    GL_STATIC_DRAW
	                );
		}
	std::cout << "nvbo... " << std::endl;
		{
			GLScope<VBO<1> > v(o.nvbo);
			glBufferData(
	                    GL_ARRAY_BUFFER,
	                    m->mNumVertices * 3 * sizeof(float),
	                    (float *)&m->mNormals[0],
	                    GL_STATIC_DRAW
	                );
		}
	std::cout << "vao... " << std::endl;
			GLScope<VAO > a(o.vao);
	        glEnableVertexAttribArray(0);
	        glEnableVertexAttribArray(1);
	        {
				GLScope<VBO<1> > v(o.vvbo);
		        glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
				glERR("opengl:vaosetup1");
	    	}
	    	{
				GLScope<VBO<1> > v(o.nvbo);
		        glVertexAttribPointer (1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
				glERR("opengl:vaosetup2");
	    	}
	    }
		{
	std::cout << "normals... " << std::endl;
			GLScope<VBO<1> > v(o.vvbo);
			glBufferData(
	                    GL_ARRAY_BUFFER,
	                    m->mNumVertices * 3 * sizeof(float),
	                    (float *)&m->mVertices[0],
	                    GL_STATIC_DRAW
	                );
	std::cout << "vao... " << std::endl;
			GLScope<VAO > a(o.vao);
	        glEnableVertexAttribArray(0);
	        glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
			glERR("opengl:vaosetup");
	    }	    {
	std::cout << "tri... " << std::endl;
	        std::vector<unsigned int> indices(m->mNumFaces * 3);
	        for (int i = 0; i < m->mNumFaces; ++ i)
	        {
	            aiFace *face = &(m->mFaces[i]);
	            indices[i * 3]     = face->mIndices[0];
	            indices[i * 3 + 1] = face->mIndices[1];
	            indices[i * 3 + 2] = face->mIndices[2];
	        }
			GLScope<VBO<1> > t(o.tvbo,GL_ELEMENT_ARRAY_BUFFER);
	        glBufferData(
	                    GL_ELEMENT_ARRAY_BUFFER,
	                    m->mNumFaces * 3 * sizeof(unsigned int),
	                    &indices[0],
	                    GL_STATIC_DRAW
	                    );
	        o.ntri = m->mNumFaces;
			glERR("opengl:indexsetup");
		}
		objects.push_back(std::move(op));
 
	}
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
	matrixsetup ms;
	ms.V = View;
	ms.M = Model;
	ms.P = Proj;
	ms.N = (ms.V*ms.M).block<3,3>(0,0).transpose();

	do {
		{
			GLScope<FBO> s(fbo);
			glClearColor(0.0,0.0,0.0,1.0);
			glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
			for(auto & o : objects)
				o->render(ms);
		}

		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		{
			trgb.bind()
			tpos.bind()
			tdepth.bind()
			// basic image render with shader activated

		}	
		glfwSwapBuffers(window);
		glfwPollEvents();
	} 
	while( glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS && glfwWindowShouldClose(window) == 0 );
	glfwTerminate();

	// DATA available in fbo
	// extract texture
	// save to image
	glfwTerminate();
}