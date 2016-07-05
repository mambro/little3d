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
	layout(location = 0) in vec4 vertexPosition_modelspace;
	layout(location = 1) in vec3 vertexNormal_modelspace;
	uniform mat4 m_VM;
	uniform mat4 m_P;
	uniform mat3 m_N;
	uniform vec3 l_pos;
	out vec3 DataIn_normal;
	out vec3 DataIn_lightDir;
	out vec3 DataIn_eye;

	void main(){
		vec4 pos = m_VM * vertexPosition_modelspace;
    	DataIn_normal = normalize(m_N * vertexNormal_modelspace);
    	DataIn_lightDir = vec3(l_pos - pos.xyz);
    	DataIn_eye = vec3(-pos);

	    gl_Position =  m_P*pos;
	}
);

const char * meshf = GLSL330(
	out vec4 color;
	uniform vec4 diffuse;
	uniform vec4 ambient;
	uniform vec4 specular;
	uniform float shininess;
	in vec3 DataIn_normal;
	in vec3 DataIn_lightDir;
	in vec3 DataIn_eye;


	void main()
	{
		vec4 spec = vec4(0.0);

		vec3 n = normalize(DataIn_normal);
		vec3 l = normalize(DataIn_lightDir);
		vec3 e = normalize(DataIn_eye);

		float intensity = max(dot(n,l), 0.0);
		if (intensity > 0.0) {
			vec3 h = normalize(l + e);
			float intSpec = max(dot(h,n), 0.0);
			spec = specular * pow(intSpec, shininess);
		}

		color = max(intensity * diffuse + spec, ambient);

	}
);


struct matrixsetup
{
	Eigen::Matrix4f MVP;
	Eigen::Matrix4f VM;
	Eigen::Matrix4f P;
	Eigen::Matrix3f N;
};

struct material
{
	void initshader()
	{
		uVM.init(sha,"m_VM");
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
		uVM << x.VM;
		uP << x.P;
		uN << x.N;
	}

	Shader sha;

	WrappedUniform<Eigen::Matrix4f> uVM;
	WrappedUniform<Eigen::Matrix4f> uP;
	WrappedUniform<Eigen::Matrix3f> uN;
	WrappedUniform<Eigen::Vector3f> l_pos;
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
	trgb.init(GLSize(width,height),true); 
	tdepth.init(GLSize(width,height),false); // float

	FBO fbo;
	{
		FBO::Setup s(fbo);
		s.attach(trgb);
		s.attach(tdepth);
		// optional: call glDrawBuffers pointing to GL_COLOR_ATTACHMENT0
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
	ms.VM = View*Model;
	ms.P = Proj;
	ms.N = ms.VM.block<3,3>(0,0).transpose();

	{
		GLScope<FBO> s(fbo);
		glClearColor(0.0,0.0,0.0,1.0);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		for(auto & o : objects)
			o->render(ms);
	}
	if(!trgb.save("color.png"))
		std::cout << "failed saveccolor\n";
	if(!tdepth.save("depth.png"))
		std::cout << "failed save depth\n";

	// DATA available in fbo
	// extract texture
	// save to image
	glfwTerminate();
}