#include "glpp/app.hpp"
#include "glpp/draw.hpp"
#include "glpp/gleigen.hpp"
#include "glpp/imageproc.hpp"
#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags
#include <Eigen/Geometry>
#include <iostream>
#include "glpp/ArcBall.hpp"

using namespace glpp;

std::string splitpath0(std::string w)
{
	return w.substr(0,w.find_last_of("/\\")+1);
}

const char * meshv = GLSL330(
	layout(location = 0) in vec4 vertexPosition_modelspace;
	layout(location = 1) in vec3 vertexNormal_modelspace;
	layout(location = 2) in vec3 vertexTexCoords;
	uniform mat4 m_VM;
	uniform mat4 m_P;
	uniform mat3 m_N;
	uniform vec3 l_pos;
	out vec3 uv;
	out vec3 DataIn_normal;
	out vec3 DataIn_lightDir;
	out vec3 DataIn_eye;

	void main(){
		vec4 pos = m_VM * vertexPosition_modelspace;
    	DataIn_normal = normalize(m_N * vertexNormal_modelspace);
    	DataIn_lightDir = vec3(l_pos - pos.xyz);
    	DataIn_eye = vec3(-pos);
    	uv = vertexTexCoords;

	    gl_Position =  m_P*pos;
	}
);

const char * meshf = GLSL330(
	out vec4 color;
	uniform vec4 diffuse;
	uniform vec4 ambient;
	uniform vec4 specular;
	uniform sampler2D tex;
	uniform float shininess;
	in vec3 uv;
	in vec3 DataIn_normal;
	in vec3 DataIn_lightDir;
	in vec3 DataIn_eye;

	vec3 uv2color(vec2 uv)
	{
		return vec3(uv.x,uv.y,0.2);
	}

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
		vec4 o = texture(tex,uv.xy);
		//o += diffuse;
		//o += vec4(uv,1.0);
		color = max(intensity * o + spec, ambient);
		color = o+ 0.00001*vec4(uv,1.0) + 0.00001*diffuse;
		//color = vec4(uv2color(uv),1.0);
	}
);


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
		uVM.init(sha,"m_VM");
		uP.init(sha,"m_P");
		uN.init(sha,"m_N");
		l_pos.init(sha,"l_pos");
		diffuse.init(sha,"diffuse");
		ambient.init(sha,"ambient");
		specular.init(sha,"specular");
		shininess.init(sha,"shininess");
		sha.uniform<int>("tex") << 0;

		l_pos << Eigen::Vector3f(0.0,2.0,3.0);
		diffuse << Eigen::Vector4f(0.0,1.0,0.0,1.0);
		ambient << Eigen::Vector4f(0.2,0.2,0.2,0.4);
		specular << Eigen::Vector4f(0.2,0.2,0.2,0.4);
		shininess << 10;
	} 

	void enter(matrixsetup & x)
	{
		Eigen::Matrix4f VM = x.V*x.M;
		sha.bind();
		uVM << VM;
		uP << x.P;
		uN << VM.block<3,3>(0,0).transpose();
		tex.bind(GL_TEXTURE_2D,0);
	}

	void exit()
	{
		tex.unbind();
		sha.unbind();
	}


	Shader sha;
	Texture tex;

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
		tevbo.init();
		vao.init();		
	}


	void render(matrixsetup & mvp)
	{
		GLScope<VAO> v(vao);
		GLScope<VBO<1> > t(tvbo,GL_ELEMENT_ARRAY_BUFFER);
		mat->enter(mvp);		
    	glDrawElements(GL_TRIANGLES,nindices,GL_UNSIGNED_INT,0); 
    	mat->exit();
	}

	
	int nindices = 0;
	VBO<1> vvbo;
	VBO<1> nvbo;
	VBO<1> tvbo;
	VBO<1> tevbo;
	VAO    vao;
	std::shared_ptr<material> mat;
};

int main(int argc, char **argv)
{
	if(argc != 2)
	{
		std::cerr << "needed filename of assimp resource\n";
		return -1;
	}
	int width = 640;
	int height = 480;
	auto window = glpp::init(width,height);
		glERR("opengl:after init");

	std::vector<std::unique_ptr<basicobj> >  objects;

	Assimp::Importer importer;
	importer.SetPropertyInteger(AI_CONFIG_PP_PTV_NORMALIZE, 1);
	std::string base = splitpath0(argv[1]);
	const aiScene* scene = importer.ReadFile( argv[1],aiProcess_Triangulate
		|aiProcess_GenNormals|aiProcess_TransformUVCoords|aiProcess_PreTransformVertices); 
	    //aiProcess_CalcTangentSpace       | 
	    //aiProcess_Triangulate            |
	    //aiProcess_JoinIdenticalVertices  |
	    //aiProcess_SortByPType);
	// If the import failed, report it
	if( !scene)
		return -1;

	// SEE ALSO: http://www.gamedev.net/topic/666478-texture-mapping-problems-with-assimp/

	std::shared_ptr<material> mat = std::make_shared<material>();
	std::cout << "only one got:" << scene->mNumMeshes << " meshes" << std::endl;
	for(int i = 0; i < scene->mNumMeshes; i++)
	{
		aiMesh * m = scene->mMeshes[i];
		aiMaterial * ma = scene->mMaterials[m->mMaterialIndex];
		aiString name;
		ma->GetTexture(aiTextureType_DIFFUSE,0,&name);
		std::cout << "Properties faces/vertices:" << m->mNumFaces << " " << m->mNumVertices << " uv:" << m->mNumUVComponents[0] << " material:" <<    m->mMaterialIndex << " texture " <<  name.C_Str() << std::endl;


		// TODO rescale
		glERR("opengl:before new");
		std::unique_ptr<basicobj> op(new basicobj());
		basicobj & o = *op;
		o.mat = mat;
		if(name.length)
		{
			std::string tname = name.C_Str();
			std::replace( tname.begin(), tname.end(), '\\', '/');
			std::cout << "name is " << tname << " in " << base << std::endl;			
			mat->tex.load(tname[0] == '/' ? tname : base+tname);
			if(mat->tex)
			{
				std::cout << "texture loaded " << mat->tex.size() << std::endl;
			}
		}
		{
			GLScope<VAO > a(o.vao);
			{
				std::cout << "vbo... " << std::endl;
				GLScope<VBO<1> > v(o.vvbo);
				glBufferData(GL_ARRAY_BUFFER,m->mNumVertices * 3 * sizeof(float),(float *)&m->mVertices[0],GL_STATIC_DRAW);
		        glEnableVertexAttribArray(0);
		        glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
			}
			if(m->HasNormals())
			{
				std::cout << "nbo... " << std::endl;
				GLScope<VBO<1> > v(o.nvbo);
				glBufferData(GL_ARRAY_BUFFER,m->mNumVertices * 3 * sizeof(float),(float *)&m->mNormals[0],GL_STATIC_DRAW);
		        glEnableVertexAttribArray(1);
		        glVertexAttribPointer (1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
			}
	    	if(m->HasTextureCoords(0))
			{
				std::cout << "tevbo... " << m->mTextureCoords[0] << std::endl;
				GLScope<VBO<1> > v(o.tevbo);
				glBufferData(GL_ARRAY_BUFFER,m->mNumVertices * 3 * sizeof(float),(float *)m->mTextureCoords[0],GL_STATIC_DRAW);
			    glEnableVertexAttribArray(2);
		        glVertexAttribPointer (2, 3, GL_FLOAT, GL_FALSE, 0, NULL);
			}
		}
		{
			std::cout << "tri... " << std::endl;
	        std::vector<unsigned int> indices(m->mNumFaces * 3);
	        for (int i = 0; i < m->mNumFaces; ++ i)
	        {
	            aiFace *face = &(m->mFaces[i]);
	            indices[i * 3 + 0] = face->mIndices[0];
	            indices[i * 3 + 1] = face->mIndices[1];
	            indices[i * 3 + 2] = face->mIndices[2];
	        }
			GLScope<VBO<1> > t(o.tvbo,GL_ELEMENT_ARRAY_BUFFER);
	        glBufferData(GL_ELEMENT_ARRAY_BUFFER,indices.size() * sizeof(unsigned int),&indices[0],GL_STATIC_DRAW);
	        o.nindices = indices.size();
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
	ArcBall hb(Eigen::Vector3f(0,0,0),0.75,window->screenToNDC());

	std::cout << "go...\n";
	//glEnable(GL_BLEND);
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
	GLImageProc imgproc;
	imgproc.init();

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
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		ms.M = hb.getTransformation();
		//imgproc.runOnScreen(objects[0]->mat->tex);
		for(auto & o : objects)
			o->render(ms);
		glfwSwapBuffers(*window);
		glfwPollEvents();
	} 
	while( glfwGetKey(*window, GLFW_KEY_ESCAPE ) != GLFW_PRESS && glfwWindowShouldClose(*window) == 0 );
	glfwTerminate();
}