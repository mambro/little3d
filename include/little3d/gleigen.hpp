#pragma once
#include <Eigen/Core>
#include <math.h>

namespace little3d
{
namespace eigen {
	// http://spointeau.blogspot.it/2013/12/hello-i-am-looking-at-opengl-3.html
	//
	// Eigen by default is column-major == OpenGL column-major
	template<class T>
	Eigen::Matrix<T,4,4> perspective
	(
	    double fovy,
	    double aspect,
	    double zNear,
	    double zFar
	)
	{
	    typedef Eigen::Matrix<T,4,4> Matrix4;

	    assert(aspect > 0);
	    assert(zFar > zNear);

	    double radf = fovy*M_PI/180;

	    double tanHalfFovy = tan(radf / 2.0);
	    Matrix4 res = Matrix4::Zero();
	    res(0,0) = 1.0 / (aspect * tanHalfFovy);
	    res(1,1) = 1.0 / (tanHalfFovy);
	    res(2,2) = - (zFar + zNear) / (zFar - zNear);
	    res(3,2) = - 1.0;
	    res(2,3) = - (2.0 * zFar * zNear) / (zFar - zNear);
	    return res;
	}

	template<class T>
	Eigen::Matrix<T,4,4> ortho
	(
	    const Eigen::Matrix<T,3,1> lbn,const Eigen::Matrix<T,3,1> rtf
	)
	{
	    typedef Eigen::Matrix<T,4,4> Matrix4;
	    Eigen::Vector3f t((rtf(0)-lbn(0))/(rtf(0)-lbn(0)),(rtf(1)-lbn(1))/(rtf(1)-lbn(1)),(rtf(2)-lbn(2))/(rtf(2)-lbn(2)));

	    Matrix4 r;
	    r << 2/(rtf(0)-lbn(0)),0,0,t(0),
	    	0,2/(rtf(1)-lbn(1)),0,t(1),
	    	0,0,-2/(rtf(2)-lbn(2)),t(2),
	    	0,0,0,1;
	    return r;
	}

	template<class T>
	Eigen::Matrix<T,4,4> lookAt
	(
	    Eigen::Matrix<T,3,1> eye,
	    Eigen::Matrix<T,3,1>  center,
	    Eigen::Matrix<T,3,1>  up
	)
	{
	    typedef Eigen::Matrix<T,4,4> Matrix4;
	    typedef Eigen::Matrix<T,3,1> Vector3;

	    Vector3 f = (center - eye).normalized();
	    Vector3 u = up.normalized();
	    Vector3 s = f.cross(u).normalized();
	    u = s.cross(f);

	    Matrix4 res;
	    res <<  s.x(),s.y(),s.z(),-s.dot(eye),
	            u.x(),u.y(),u.z(),-u.dot(eye),
	            -f.x(),-f.y(),-f.z(),f.dot(eye),
	            0,0,0,1;

	    return res;
	}

	// pose of the camera WRT the origin => camera world inverse with Z flipped AKA 
	template<class T>
	Eigen::Matrix<T,4,4> camerapose2eye(Eigen::Matrix<T,4,4> pose)
	{
		Eigen::Matrix<T,4,4> q(pose.inverse());
		q(2,0) =-q(2,0);
		q(2,1) =-q(2,1);
		q(2,2) =-q(2,2);
		q(2,3) =-q(2,3);
		return q;
	}

	// ALIAS deprecated
	template<class T>
	Eigen::Matrix<T,4,4> pose2camera(Eigen::Matrix<T,4,4> pose)
	{
		return camerapose2eye(pose);
	}
	}
}