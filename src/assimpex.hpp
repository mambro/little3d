#pragma once

namespace glpp
{

inline std::string splitpath0(std::string w)
{
	return w.substr(0,w.find_last_of("/\\")+1);
}

namespace StandardShader
{
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
}

struct matrixsetup
{
	Eigen::Matrix4f VM;
	Eigen::Matrix4f M;
	Eigen::Matrix4f V;
	Eigen::Matrix4f P;
	Eigen::Matrix3f N;
};

struct material
{
	void initshader()
	{
		uM.init(sha,"m_M");
		uVM.init(sha,"m_VM");
		uP.init(sha,"m_P");
		uN.init(sha,"m_N");
		l_pos.init(sha,"l_pos");
		diffuse.init(sha,"diffuse",false);
		ambient.init(sha,"ambient",false);
		specular.init(sha,"specular",false);
		shininess.init(sha,"shininess",false);
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
		uM << x.M;
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

	WrappedUniform<Eigen::Matrix4f> uVM,uM,uP;
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


inline int assimploader(const char * name, std::vector<std::unique_ptr<basicobj> > & objects, std::shared_ptr<material> & mat)
{

	Assimp::Importer importer;
	importer.SetPropertyInteger(AI_CONFIG_PP_PTV_NORMALIZE, 1);
	std::string base = splitpath0(name);
	const aiScene* scene = importer.ReadFile( name,aiProcess_Triangulate
		|aiProcess_GenNormals|aiProcess_TransformUVCoords|aiProcess_PreTransformVertices); 
	    //aiProcess_CalcTangentSpace       | 
	    //aiProcess_Triangulate            |
	    //aiProcess_JoinIdenticalVertices  |
	    //aiProcess_SortByPType);
	// If the import failed, report it
	if( !scene)
	{
		std::cout << "cannot load file\n";
		return -1;
	}
	else
		std::cout << "loaded:" << scene->mNumMeshes << " meshes" << std::endl;

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
	return 0;
}
}