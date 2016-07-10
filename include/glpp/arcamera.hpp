#pragma once
#include <Eigen/Dense>

struct CameraIntrinsics
{
	float fx,fy,cx,cy;
	float dist[5] = {0,0,0,0,0}; // distorsion
	int width=0,height=0;

	void fromK(Eigen::Matrix3f K);
	Eigen::Matrix3f getK() const;
	Eigen::Matrix4f getK4(float near, float far) const;

	Eigen::Matrix4f getGLProjection(float near, float far, bool flipimage = false) const;

};

/// pose to camera
inline Eigen::Matrix4f GLflipviewport(int h)
{
	 Eigen::Matrix4f r;
	 r << 1,0,0,0,  0,-1,0,h,     0,0,1,0,   0,0,0,1;
	 return r;
}


/// clip to viewport 
inline Eigen::Matrix4f GLclip2viewport(int w, int h, int originx = 0, int originybottom = 0)
{
	float r_f=1.0;
	float r_n=0.0;
	
	float aa = (r_f-r_n)/2;
	float bb = (r_n+r_f)/2;
	Eigen::Matrix4f r;
	r << w/2,0,0,originx+w/2, 0,h/2,0,originybottom+h/2,  0,0,aa,bb,   0,0,0,1;
	return r;
}


inline Eigen::Matrix4f GLscaleViewport(float s)
{
	Eigen::Matrix4f r = Eigen::Matrix4f::Identity();
	r(0,0) *= s;
	r(1,1) *= s;
	return r;
}

/// clip to viewport  FLIP Z
inline Eigen::Matrix4f GLworld2eye()
{
	 Eigen::Matrix4f r;
	 r << 1,0,0,0,    0,1,0,0,    0,0,-1,0,    0,0,0,1;
	 return r;	
}

inline void CameraIntrinsics::fromK(Eigen::Matrix3f K)
{
	fx = K(0,0);
	fy = K(1,1);
	cx = K(0,2);
	cy = K(1,2);
}

inline Eigen::Matrix3f CameraIntrinsics::getK() const
{
	Eigen::Matrix3f r;
	r << fx , 0, cx, 0, fy, cy, 0,0,1;
	return r;
}

inline Eigen::Matrix4f CameraIntrinsics::getK4(float n, float f) const
{
	Eigen::Matrix4f r;
	float a = f/(f - n);       // note: OpenGL is (f+n)/(n-f);
	float b = -(f*n)/(f - n);  // note: OpenGL is 2*f*n/(n-f);
	r << fx , 0, cx,0,    0, fy, cy,0,    0,0,a,b,   0,0,1,0; // minus sign due to z pos
	return r;
}

inline Eigen::Matrix4f CameraIntrinsics::getGLProjection(float near, float far, bool flipimage) const
{
	float w = width;
	float h = height;
	Eigen::Matrix4f r;
	r << 2*fx/w, 0, 1-(2*cx)/w, 0,      	
		 0, 2*fy/h,1-2*cy/h,0,    
		 0,0,1-2*far/(far-near),-2*far*near/(far-near),      
		 0,0,-1,0;
	if(flipimage)
	{
		r(1,1) = -r(1,1);
		r(1,2) = -r(1,2);
	}
	return r;
}
