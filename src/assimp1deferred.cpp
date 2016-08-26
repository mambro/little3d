// See: http://aras-p.info/texts/CompactNormalStorage.html#method00xyz
// http://learnopengl.com/#!Advanced-Lighting/Deferred-Shading
// http://www.ogldev.org/www/tutorial36/tutorial36.html
// http://gamedevs.org/uploads/deferred-shading-tutorial.pdf
#include "little3d/little3d.hpp"
#include "little3d/assimpex.hpp"



struct Light {
    Eigen::Vector3f position;
    Eigen::Vector3f dir;
    Eigen::Vector3f color;
    
    float linear;
    float quadratic;
};


using namespace little3d;

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
	layout (location = 1) out vec4 gNormal;
	layout (location = 2) out vec4 gPosition;

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
		if(textureSize(tex,0).x == 0)
			o = vec4(1.0,0,0,1.0);
		//o += diffuse;
		//o += vec4(uv,1.0);
		//color = max(intensity * o + spec, ambient);
		//color = o+ 0.00001*vec4(uv,1.0) + 0.00001*diffuse;

	    // Store the fragment position vector in the first gbuffer texture
	    gPosition = vec4(FragPos,1.0);
	    // Also store the per-fragment normals into the gbuffer
	    gNormal = vec4(normalize(DataIn_normal),1.0);
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
    lighting += (diffuse + specular)*attenuation;
    FragColor = vec4(lighting,1);

}
);


const char * defshadowshaf = GLSL330(

out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;
uniform vec3 viewPos;

void main()
{             
    vec3 FragPos = texture(gPosition,   TexCoords).rgb;
    vec3 Normal  = texture(gNormal,     TexCoords).rgb; // TODO: optimize decode
    vec4 DiSpec  = texture(gAlbedoSpec, TexCoords);
    vec3 Diffuse = DiSpec.rgb;
    float Specular = DiSpec.a;
    
    vec3 lighting  = Diffuse * 0.8; // hard-coded ambient component
    vec3 viewDir  = normalize(viewPos - FragPos);

	

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
    lighting += (diffuse + specular)*attenuation;
    FragColor = vec4(lighting,1);

}
);

// http://www.codinglabs.net/tutorial_opengl_deferred_rendering_shadow_mapping.aspx
// http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-16-shadow-mapping/
class DeferredShadow
{
public:
	FBO fbo;
	DepthTexture tdepth;
	Eigen::Matrix4f view; 	// for the shadow capture as the virtual camera - computed by updateMatrix
	Eigen::Matrix4f proj; 	// for the shadow capture as the virtual camera - computed by updateMatrix
	Eigen::Matrix4f lightMat; // for being used inside final pass - computed by updateMatrix
	GLSize size;

	DeferredShadow(GLSize a) : size(a) {}
	void updateMatrix(const Light & l)
	{
		view = eigen::lookAt(l.position,Eigen::Vector3f(l.position+l.dir),Eigen::Vector3f(0,1,0));
		proj = eigen::ortho(Eigen::Vector3f(-10,10,-10),Eigen::Vector3f(10,-10,10));

		// converts from the NDC to the texture space
		Eigen::Matrix4f biasMatrix;
		biasMatrix <<
		0.5, 0.0, 0.0, 0.0,
		0.0, 0.5, 0.0, 0.0,
		0.0, 0.0, 0.5, 0.0,
		0.5, 0.5, 0.5, 1.0;
		lightMat = biasMatrix * proj * view;

	}

	void init()
	{
		tdepth.init(size,false); // float
		{
			FBO::Setup s(fbo);
			s.attach(tdepth);
		}
	}

	void capture(std::function<void(const Eigen::Matrix4f&p,const Eigen::Matrix4f&v)> fx)
	{
		// TODO: alter the matrices to use this CUSTOM view
		GLScope<FBO> s(fbo);
		fx(proj,view);		
	}
};

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


struct DeferredState
{
	FBO fbo;
	GLSize size_;
	ColorTexture trgb;
	ColorTexture tnormal;
	ColorTexture tpos;

	DeferredState(	GLSize s): size_(s)
	{
		// prepare textues
		trgb.init(size_,true,false); 
		tnormal.init(size_,true,true);
		tpos.init(size_,true,true); 
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
			s.attach(tnormal,1); //  first is normal
			s.attach(trgb,0);
			s.attach(tpos,2);
			s.makedepth();
		}	
	}
};

struct ViewportGeo
{
	VAO vao;
	VBO<1> vvbo;

	ViewportGeo()
	{
		vao.init();
		vvbo.init();
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
	}
};

struct DeferredRenderer
{
	ViewportGeo geo;
	DepthTexture * tshadow = 0;
	Eigen::Matrix4f proj; 
	Eigen::Matrix4f view; 
	Eigen::Matrix4f lightMat;
	Shader sha;
	WrappedUniform<Eigen::Vector3f> uviewPos;
	WrappedUniform<Eigen::Matrix4f> ulightMat;

	DeferredRenderer()
	{


		{
//		    if(!sha.load(defshav, defshaf, 0, 0, 0, false))
		    if(!sha.load("deferredv.glsl", "deferredf.glsl",0, 0, 0, true))
		    {
		    	std::cout << "failed Deferred shader" << std::endl;
		    	exit(-1);
		    }
		    // link the input uniforms for the textures FOR OUTPUT
		    GLScope<Shader> ss(sha);
		    sha.uniform<int>("gAlbedoSpec") << 0;
		    sha.uniform<int>("gNormal") << 1;
		    sha.uniform<int>("gPosition") << 2;
		    sha.uniform<int>("gShadow") << 3;
		}

		uviewPos = sha.uniform<Eigen::Vector3f>("viewPos");
		ulightMat = sha.uniform<Eigen::Matrix4f>("lightMat");


//            GLScope<VBO<1>> xvbo(tvbo, GL_ELEMENT_ARRAY_BUFFER);
//            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements, GL_STATIC_DRAW);
	}

	void setLight(int index, Light l)
	{
		GLScope<Shader> x(sha);
		std::string prefix = "lights[" + std::to_string(index) + "].";

		sha.uniform<Eigen::Vector3f>(prefix + "Position") << l.position;
		sha.uniform<Eigen::Vector3f>(prefix + "Color") << l.color;
		sha.uniform<float>(prefix + "Linear") << l.linear;
		sha.uniform<float>(prefix + "Quadratic") << l.quadratic;
	}

	void render(DeferredState & ds)
	{

        GLScope<VAO> xvao(geo.vao);
        GLScope<Shader> xsha(sha);  
        GLScope<Texture> t1(ds.trgb,   GL_TEXTURE_2D,0);
        GLScope<Texture> t3(ds.tnormal,GL_TEXTURE_2D,1);
        GLScope<Texture> t2(ds.tpos,   GL_TEXTURE_2D,2);
        GLScope<Texture> t4(*tshadow,GL_TEXTURE_2D,3);
    	uviewPos << view.block<3,1>(0,3);
    	ulightMat << lightMat;
        GLScopeDisable<GL_DEPTH_WRITEMASK> xdw;
        GLScopeDisable<GL_DEPTH_TEST> xdt; // no need to write or read depth
        //glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        //GLScope<VBO<1>> xvbo(tvbo_, GL_ELEMENT_ARRAY_BUFFER);
        //glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}

	void capture(DeferredState & ds, std::function<void(const Eigen::Matrix4f&p,const Eigen::Matrix4f&v)> fx)
	{
		GLScope<FBO> s(ds.fbo);
		fx(proj,view);		
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
	auto window = little3d::init(width,height,"hello deferred");
	DeferredState defs(window->viewportSize);
	DeferredRenderer defr;
	DeferredShadow shadow(window->viewportSize);
	defr.tshadow = &shadow.tdepth;
	shadow.init();
	glERR("opengl:after deferred");

	std::vector<std::unique_ptr<basicobj> >  objects;
	std::vector<std::shared_ptr<material> > materials;
	bool dosave = false;
	bool donormalize = true;
	for(int i = 1; i < argc; i++)
		if(strcmp(argv[i],"--save") == 0)
			dosave = true;
		else if(strcmp(argv[i],"--normalize") == 0)
			donormalize = true;
		else if(strcmp(argv[i],"--nonormalize") == 0)
			donormalize = false;
		else
			assimploader(argv[i],objects,materials,donormalize);


	Eigen::Matrix4f matplane;
	matplane << 1.0,0.0,0.0,0.0, 
		0.0,0.0,1.0,0.0,
		0.0,1.0,0.0,0.0,
		0.0,0.0,0.0,1.0;
	//addplane(objects,materials,matplane,Eigen::Vector2f(10,10));

	// ATTACH shader
	std::shared_ptr<Shader> sha = std::make_shared<Shader>();
	sha->load(meshv, meshf, 0, 0, 0, false);

	for(auto & it : materials)
	{
		it->sha = sha;
	    GLScope<Shader> ss(*sha);
		it->initshader();
		glERR("opengl:setup");
	}


	std::cout << "go...\n";
	glEnable(GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	Light l;
	l.dir = Eigen::Vector3f(0,0,-1);
	l.position = Eigen::Vector3f(0.0,0.0,2.0); // fixed in world coordinates
	l.color = Eigen::Vector3f(0.1,0.1,0.0);
	l.linear = 0.01;
	l.quadratic = 0.03;
	defr.setLight(0,l);
	shadow.updateMatrix(l);
	defr.lightMat = shadow.lightMat;

	auto Proj = little3d::eigen::perspective<float>(60.0f,         // The horizontal Field of View, in degrees : the amount of "zoom". Think "camera lens". Usually between 90° (extra wide) and 30° (quite zoomed in)
	    width/(float)height, // Aspect Ratio. Depends on the size of your window. Notice that 4/3 == 800/600 == 1280/960, sounds familiar ?
	    0.1f,        // Near clipping plane. Keep as big as possible, or you'll get precision issues.
	    300.0f       // Far clipping plane. Keep as little as possible.
	);
	auto View      = little3d::eigen::lookAt<float>({0,2,2},{0,0,0},{0,1,0});
	Eigen::Matrix4f Model = Eigen::Matrix4f::Identity();
	Model.block<3,3>(0,0) = Eigen::Matrix3f::Identity();
	std::cout << "Proj is\n" << Proj  << std::endl;
	std::cout << "View is\n" <<  View  << std::endl;
	std::cout << "Model is\n" <<  Model << std::endl;
	std::cout << "Matrix is\n" << Proj * View * Model << std::endl;



	ArcBall hb(Eigen::Vector3f(0,0,0),0.75,window->screenToNDC());
	hb.attach(window);


	auto fx = [&hb,&objects] (const Eigen::Matrix4f & proj, const Eigen::Matrix4f & view) {
			matrixsetup ms;
			ms.V = view;
			ms.M = hb.getTransformation();
			ms.P = proj;
	        glClearColor(0.0,0.0,0.0,1.0);
			glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
			for(auto & o : objects)
				o->render(ms);
	};

	do {

		glERR("opengl:pre defrender");
		// render to shadow
		shadow.capture(fx);

		// prepare deferred
		defr.view = View;
		defr.proj = Proj;
		defr.capture(defs,fx);

		glERR("opengl:pre defrender");
		defr.lightMat = shadow.lightMat;
		defr.render(defs);
		glERR("opengl:after def render");

		glfwSwapBuffers(*window);
		glfwPollEvents();
		if(dosave)
			break;
	} 
	while( glfwGetKey(*window, GLFW_KEY_ESCAPE ) != GLFW_PRESS && glfwWindowShouldClose(*window) == 0 );
	
	if(dosave)
	{
		if(!defs.trgb.save("color.png"))
			std::cout << "failed saveccolor\n";
		if(!defs.tnormal.save("normal.png"))
			std::cout << "failed save normal\n";
		if(!defs.tpos.save("pos.png"))
			std::cout << "failed save tpos\n";
	}
	glfwTerminate();

}