// See https://www.shadertoy.com/view/4sj3WK
#include "glpp/app.hpp"
#include "glpp/draw.hpp"
#include <iostream>

using namespace glpp;

struct tbox
{
	float vmin[3];
	float vmax[3];
};

struct tsphere
{
	float center[3];
	float radius;
};

const float SQUARE[] = {
    -1.0f,  1.0f,
    -1.0f, -1.0f,
     1.0f,  1.0f,
     1.0f, -1.0f
};


const char * meshv = GLSL330(
	layout(location = 0) in vec2 point;

	void main(){
	    gl_Position =  vec4(point, 0.0, 1.0);
	}
);

const char * meshf = GLSL330(

	struct box
	{
		vec3 min;
		vec3 max;
	};

	struct sphere
	{
		vec3 center;
		float radius;
	};

	struct intersectout
	{
		float lambda;
		vec3 point;
		vec3 normal;
	};

	uniform vec3      cameraCenter;
	uniform box       box1;
	uniform vec3      iResolution;           // viewport resolution (in pixels)
	uniform float     iGlobalTime;           // shader playback time (in seconds)
	uniform sphere sphere1;
	out vec4 xgl_FragColor;

	/// returns normal and lambda of point
	intersectout intersectSphere(vec3 ray, vec3 dir, vec3 center, float radius)
	{
		vec3 rc = ray-center;
		float c = dot(rc, rc) - (radius*radius);
		float b = dot(dir, rc);
		float d = b*b - c;
		float t = -b - sqrt(abs(d));
		float st = step(0.0, min(t,d));
		float outt = mix(-1.0, t, st);
		intersectout xout;
		xout.lambda = outt;
		xout.point = (ray+dir*outt);
		xout.normal = normalize(center - xout.point); 
		return xout;
	}

	// complex background
	vec3 background(float t, vec3 rd)
	{
	 vec3 light = normalize(vec3(sin(t), 0.6, cos(t)));
	 float sun = max(0.0, dot(rd, light));
	 float sky = max(0.0, dot(rd, vec3(0.0, 1.0, 0.0)));
	 float ground = max(0.0, -dot(rd, vec3(0.0, 1.0, 0.0)));
	 return 
	  (pow(sun, 256.0)+0.2*pow(sun, 2.0))*vec3(2.0, 1.6, 1.0) +
	  pow(ground, 0.5)*vec3(0.4, 0.3, 0.2) +
	  pow(sky, 1.0)*vec3(0.5, 0.6, 0.7);
	}

#if 0
	// assumes (u,v) in [0,1]. Divides the ares into nTile x nTile squares.
	vec3 checkerBoard(vec2 uv, float nTile) {
		vec3 color1 = vec3(1.0, 1.0, 1.0);
		vec3 color2 = vec3(0.0, 0.0, 0.0);
		float side = 1.0/nTile;
		float c1 = mod(uv.x, 2.0 * side);
		c1 = step(side, c1);
		float c2 = mod(uv.y, 2.0 * side);
		c2 = step(side, c2);	
		vec3 color = mix(color1, color2, mod(c1+c2,2.0));
		return color;
	}

	vec2 intersectBox(vec3 origin, vec3 dir, const box b) {
		vec3 tMin = (b.min - origin) / dir;
		vec3 tMax = (b.max - origin) / dir;
		vec3 t1 = min(tMin, tMax);
		vec3 t2 = max(tMin, tMax);
		float tNear = max(max(t1.x, t1.y), t1.z);
		float tFar = min(min(t2.x, t2.y), t2.z);
		return vec2(tNear, tFar);
	}
#endif

	void main(void)
	{
	// fragment to UV
	 vec2 uv = (-1.0 + 2.0*gl_FragCoord.xy / iResolution.xy) * vec2(iResolution.x/iResolution.y, 1.0);

	 // TODO full projective camera
	 vec3 ro = cameraCenter;
	 vec3 rd = normalize(vec3(uv, 1.0));

	 // TODO box and multiple spheres
	 // TODO multiple passes for reflections
	 intersectout rs = intersectSphere(ro, rd, sphere1.center, sphere1.radius);
	 vec3 bgCol = background(iGlobalTime, rd);
	 rd = reflect(rd, rs.normal);

	 // TODO flexible background
	 vec3 col = background(iGlobalTime, rd) * vec3(0.9, 0.8, 1.0); // reflected ray
	 xgl_FragColor = vec4( mix(bgCol, col, step(0.0, rs.lambda)), 1.0 );   
	}

);

int main(int argc, char **argv)
{
	int width = 640;
	int height = 480;
	auto window = glpp::init(width,height);
		glERR("opengl:after init");

	VBO<1> vvbo;
	VAO vao;
	vvbo.init();
	vao.init();
	Shader sha;
	std::cout << "vao " << vao << " vbo " << vvbo << std::endl;
	{
	    if(!sha.load(meshv, meshf, 0, 0, 0, false))
	    	return -1;
	}
	{
		GLScope<VBO<1> > t(vvbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(SQUARE), SQUARE, GL_STATIC_DRAW);
	}
	{
		glERR("opengl:pre vao setup");
		GLScope<VAO> v(vao);
		glERR("opengl:vao bind");
		GLScope<VBO<1>> p(vvbo);
		glERR("opengl:vbo bind");
	 	glEnableVertexAttribArray(0);
		glERR("opengl:enable attrib");
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glERR("opengl:vertex attrib");
	}

	std::cout << "shader is " << sha << std::endl;
	int uResolution = glGetUniformLocation(sha, "iResolution");
	int uTime = glGetUniformLocation(sha, "iGlobalTime");
	int uCameraCenter = glGetUniformLocation(sha,"cameraCenter");
	int uSphere1c = glGetUniformLocation(sha,"sphere1.center");
	int uSphere1r = glGetUniformLocation(sha,"sphere1.radius");
	tsphere xsphere1 = {{0,0,0},1.0};
	float xtime = 0;
	float xCameraCenter[] = {0.0f,0.0f,-3.0f};
	float xResolution[] = {(float)width,(float)height,0.0f};
	{
		GLScope<Shader> sh(sha);		
		glUniform3fv(uResolution,1,xResolution);
		glUniform3fv(uCameraCenter,1,xCameraCenter);
		glUniform3fv(uSphere1c,1,xsphere1.center);
		glUniform1f(uSphere1r,xsphere1.radius);
	}

	do {
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

		{
			xtime = glfwGetTime();
			GLScope<VAO> va(vao);
			GLScope<Shader> sh(sha);
			glERR("opengl:scoppe");
			glUniform1f(uTime, xtime);
			glERR("opengl:uniform");
			glDrawArrays(GL_TRIANGLE_STRIP, 0, sizeof(SQUARE)/sizeof(SQUARE[0]) / 2);
			glERR("opengl:draw");
		}

		glfwSwapBuffers(*window);
		glfwPollEvents();
	} 
	while( glfwGetKey(*window, GLFW_KEY_ESCAPE ) != GLFW_PRESS && glfwWindowShouldClose(*window) == 0 );
	glfwTerminate();
}