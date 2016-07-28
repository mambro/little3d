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
};

struct material
{
	void initshader()
	{
		uM.init(*sha,"m_M");
		uVM.init(*sha,"m_VM");
		uP.init(*sha,"m_P");
		uN.init(*sha,"m_N");
		l_pos.init(*sha,"l_pos"); // TBR
		udiffuse.init(*sha,"diffuse",false);
		ambient.init(*sha,"ambient",false);
		specular.init(*sha,"specular",false);
		shininess.init(*sha,"shininess",false);
		sha->uniform<int>("tex") << 0;

		l_pos << Eigen::Vector3f(0.0,2.0,3.0);
		udiffuse << Eigen::Vector4f(0.0,1.0,0.0,1.0);
		ambient << Eigen::Vector4f(0.2,0.2,0.2,0.4);
		specular << Eigen::Vector4f(0.2,0.2,0.2,0.4);
		shininess << 10;
	} 

	void enter(matrixsetup & x)
	{
		Eigen::Matrix4f VM = x.V*x.M;
		sha->bind();
		uM << x.M;
		uVM << VM;
		uP << x.P;
		udiffuse << cdiffuse;
		uN << x.M.block<3,3>(0,0).transpose(); // normal in world space
		if(dualsided)
			glDisable(GL_CULL_FACE);
		if(tex)
			tex.bind(GL_TEXTURE_2D,0);
	}

	void exit()
	{
		if(dualsided)
			glEnable(GL_CULL_FACE);
		tex.unbind();
		sha->unbind();
	}


	std::shared_ptr<Shader> sha;
	Texture tex;
	bool dualsided = false;

	Eigen::Vector4f cdiffuse = Eigen::Vector4f(0.8,0.8,0.8,1.0);
	WrappedUniform<Eigen::Matrix4f> uVM,uM,uP;
	WrappedUniform<Eigen::Matrix3f> uN;
	WrappedUniform<Eigen::Vector3f> l_pos;
	WrappedUniform<Eigen::Vector4f> udiffuse,ambient,specular;
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


/// add plane, todo generalzie 
inline void addplane(std::vector<std::unique_ptr<basicobj> > & objects, std::vector<std::shared_ptr<material> > & materials, Eigen::Matrix4f pose, Eigen::Vector2f size)
{
	std::unique_ptr<basicobj> op(new basicobj());
	basicobj & o = *op;
	std::shared_ptr<material> mat = std::make_shared<material>();
	materials.push_back(mat);
	o.mat = mat;
	mat->dualsided = true;

	Eigen::Matrix<float,4,3+3+2> attribs; // vertex normal coords, stored by ..
	Eigen::Matrix<unsigned int,1,6> indices;
	indices << 0,1,2,1,2,3;

	Eigen::Vector3f x =  pose.col(0).segment<3>(0);
	Eigen::Vector3f y =  pose.col(1).segment<3>(0);
	Eigen::Vector3f z =  pose.col(2).segment<3>(0);

std::cout << "attribs is " << attribs << std::endl;

	// build a frame for the plane 
	attribs.block<1,3>(0,0) = x*size(0)/2+y*size(1)/2;
	attribs.block<1,3>(1,0) = x*size(0)/2-y*size(1)/2;
	attribs.block<1,3>(2,0) = -x*size(0)/2-y*size(1)/2;
	attribs.block<1,3>(3,0) = -x*size(0)/2+y*size(1)/2;

	for(int i = 0; i < 4; i++)
		attribs.block<1,3>(i,3) = z;

	attribs.block<1,2>(0,5) = Eigen::Vector2f(0,0);
	attribs.block<1,2>(1,5) = Eigen::Vector2f(0,1);
	attribs.block<1,2>(2,5) = Eigen::Vector2f(1,1);
	attribs.block<1,2>(3,5) = Eigen::Vector2f(1,0);

	std::cout << "attribs is " << attribs << std::endl;


	{
		GLScope<VAO > a(o.vao);
		{
			GLScope<VBO<1> > v(o.vvbo);
			glBufferData(GL_ARRAY_BUFFER,attribs.size() * sizeof(float),attribs.data(),GL_STATIC_DRAW);
	        glEnableVertexAttribArray(0);
	        glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	        glEnableVertexAttribArray(1);
	        glVertexAttribPointer (1, 3, GL_FLOAT, GL_FALSE, 0, (void*)(3*sizeof(float)));
	        glEnableVertexAttribArray(2);
	        glVertexAttribPointer (2, 2, GL_FLOAT, GL_FALSE, 0, (void*)(5*sizeof(float)));
		}
	}
	{
		GLScope<VBO<1> > t(o.tvbo,GL_ELEMENT_ARRAY_BUFFER);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,indices.size() * sizeof(unsigned int),indices.data(),GL_STATIC_DRAW);
        o.nindices = indices.size();
		glERR("opengl:indexsetup");
	}
	objects.push_back(std::move(op));	
}

inline int assimploader(const char * name, std::vector<std::unique_ptr<basicobj> > & objects, std::vector<std::shared_ptr<material> > & materials, bool donormalize = false)
{

	Assimp::Importer importer;
	if(donormalize)
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
		std::cout << "\tProperties faces/vertices:" << m->mNumFaces << " " << m->mNumVertices << " uv:" << m->mNumUVComponents[0] << " material:" <<    m->mMaterialIndex << " texture " <<  name.C_Str() << std::endl;

		// TODO rescale
		glERR("opengl:before new");
		std::unique_ptr<basicobj> op(new basicobj());
		basicobj & o = *op;
		std::shared_ptr<material> mat = std::make_shared<material>();
		materials.push_back(mat);
		o.mat = mat;
		if(name.length)
		{
			std::string tname = name.C_Str();
			std::replace( tname.begin(), tname.end(), '\\', '/');
			std::cout << "\tname is " << tname << " in " << base << std::endl;			
			mat->tex.load(tname[0] == '/' ? tname : base+tname);
			if(mat->tex)
			{
				std::cout << "\ttexture loaded " << mat->tex.size() << std::endl;
			}
		}
		{
			GLScope<VAO > a(o.vao);
			{
				std::cout << "\tvbo... " << std::endl;
				GLScope<VBO<1> > v(o.vvbo);
				glBufferData(GL_ARRAY_BUFFER,m->mNumVertices * 3 * sizeof(float),(float *)&m->mVertices[0],GL_STATIC_DRAW);
		        glEnableVertexAttribArray(0);
		        glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
			}
			if(m->HasNormals())
			{
				std::cout << "\tnbo... " << std::endl;
				GLScope<VBO<1> > v(o.nvbo);
				glBufferData(GL_ARRAY_BUFFER,m->mNumVertices * 3 * sizeof(float),(float *)&m->mNormals[0],GL_STATIC_DRAW);
		        glEnableVertexAttribArray(1);
		        glVertexAttribPointer (1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
			}
	    	if(m->HasTextureCoords(0))
			{
				std::cout << "\ttevbo... " << m->mTextureCoords[0] << std::endl;
				GLScope<VBO<1> > v(o.tevbo);
				glBufferData(GL_ARRAY_BUFFER,m->mNumVertices * 3 * sizeof(float),(float *)m->mTextureCoords[0],GL_STATIC_DRAW);
			    glEnableVertexAttribArray(2);
		        glVertexAttribPointer (2, 3, GL_FLOAT, GL_FALSE, 0, NULL);
			}
		}
		{
			std::cout << "\ttri... " << std::endl;
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