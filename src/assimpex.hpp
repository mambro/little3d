#pragma once

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
		sha.uniform<int>("tex") << 0;

		l_pos << Eigen::Vector3f(0.0,2.0,3.0);
		diffuse << Eigen::Vector4f(0.0,1.0,0.0,1.0);
		ambient << Eigen::Vector4f(0.2,0.2,0.2,0.4);
		specular << Eigen::Vector4f(0.2,0.2,0.2,0.4);
		shininess << 10;
	} 

	void enter(matrixsetup & x)
	{
		sha.bind();
		uVM << x.VM;
		uP << x.P;
		uN << x.N;
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
