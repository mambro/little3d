/** 
 * Emanuele Ruffaldi Eigen port 2016
 */
#pragma once


#include <stdint.h>
#include <Eigen/Dense>
#include <Eigen/Geometry>

namespace little3d {

/// A reimplementation of Ken Shoemake's arcball camera. SCIRun 4's camera
/// system is based off of Ken's code. The Code appears in Graphics Gems 4, 
/// III.1.
/// Unless specified otherwise, all calculations and variables stored in this
/// class are relative to the target coordinate system (TCS) for which there is
/// a transformation from screen space to TCS given by the screenToTCS
/// constructor parameter.
/// If the screenToTCS parameter in the constructor is left as the identity
/// matrix then all values are given in screen coordinates.
/// Screen coordinates are (x \in [-1,1]) and (y \in [-1,1]) where (0,0) is the
/// center of the screen.
class ArcBall
{
public:
  /// \param center         Center of the arcball in TCS (screen coordinates if 
  ///                       screenToTCS = identity). Generally this will 
  ///                       always be (0,0,0). But you may move the center
  ///                       in and out of the screen plane to various effect.
  /// \param radius         Radius in TCS. For screen coordinates, a good
  ///                       default is 0.75.
  /// \param screenToTCS    Transformation from screen coordinates
  ///                       to TCS. \p center and \p radius are given in TCS.
  ArcBall(const Eigen::Vector3f& center, float radius,
          const Eigen::Matrix4f& screenToTCS = Eigen::Matrix4f());
  
  /// Initiate an arc ball drag given the mouse click in screen coordinates.
  /// \param mouseScreenCoords  Mouse screen coordinates.
  void beginDrag(const Eigen::Vector2f& mouseScreenCoords);

  /// Informs the arcball when the mouse has been dragged.
  /// \param mouseScreenCoords  Mouse screen coordinates.
  void drag(const Eigen::Vector2f& mouseScreenCoords);

  template <class T>
  void attach(T); // attach to window

  /// Retrieves the current transformation in TCS.
  /// Obtains full transformation of object in question. If the arc ball is 
  /// being used to control camera rotation, then this will contain all
  /// concatenated camera transformations. The current state of the camera
  /// is stored in the quaternions mQDown and mQNow. mMatNow is calculated
  /// from mQNow.
  Eigen::Matrix4f getTransformation() const { return mMatNow; }

private:

  /// Calculates our position on the ArcBall from 2D mouse position.
  /// \param tscMouse   TSC coordinates of mouse click.
  Eigen::Vector3f mouseOnSphere(const Eigen::Vector3f& tscMouse);

  Eigen::Vector3f     mCenter;        ///< Center of the arcball in target coordinate system.
  float  mRadius;        ///< Radius of the arcball in target coordinate system.

  Eigen::Quaternionf     mQNow;          ///< Current state of the rotation taking into account mouse.
                                ///< Essentially QDrag * QDown (QDown is a applied first, just
                                ///< as in matrix multiplication).
  Eigen::Quaternionf     mQDown;         ///< State of the rotation since mouse down.
  Eigen::Quaternionf     mQDrag;         ///< Dragged transform. Knows nothing of any prior 
                                ///< transformations.

  Eigen::Vector3f     mVNow;          ///< Most current TCS position of mouse (during drag).
  Eigen::Vector3f     mVDown;         ///< TCS position of mouse when the drag was begun.
  Eigen::Vector3f     mVSphereFrom;   ///< vDown mapped to the sphere of 'mRadius' centered at 'mCenter' in TCS.
  Eigen::Vector3f     mVSphereTo;     ///< vNow mapped to the sphere of 'mRadius' centered at 'mCenter' in TCS.

  Eigen::Matrix4f     mMatNow;        ///< Matrix representing the current rotation.

  /// Transform from screen coordinates to the target coordinate system.
  Eigen::Matrix4f     mScreenToTCS;
};

//------------------------------------------------------------------------------
inline ArcBall::ArcBall(const Eigen::Vector3f& center, float radius, const Eigen::Matrix4f& screenToTCS) :
    mCenter(center),
    mRadius(radius),
    mScreenToTCS(screenToTCS)
{
  // glm uses the following format for quaternions: w,x,y,z.
  //        w,    x,    y,    z
  Eigen::Quaternionf qOne(1.0, 0.0, 0.0, 0.0);
  Eigen::Vector3f vZero(0.0, 0.0, 0.0);
  mMatNow.setIdentity();
  mVDown    = vZero;
  mVNow     = vZero;
  mQDown    = qOne;
  mQNow     = qOne;
}

//------------------------------------------------------------------------------
inline Eigen::Vector3f ArcBall::mouseOnSphere(const Eigen::Vector3f& tscMouse)
{
  // (m - C) / R
  Eigen::Vector3f ballMouse((tscMouse.x()  - mCenter.x() ) / mRadius,(tscMouse.y()  - mCenter.y() ) / mRadius,0.0);

  float mag = ballMouse.squaredNorm();
  if (mag > 1.0)
  {
    // Since we are outside of the sphere, map to the visible boundary of
    // the sphere.
    ballMouse *= 1.0 / sqrtf(mag);
  }
  else
  {
    // We are not at the edge of the sphere, we are inside of it.
    // Essentially, we are normalizing the vector by adding the missing z
    // component.
    ballMouse.z()  = sqrtf(1.0 - mag);
  }

  return ballMouse;
}

//------------------------------------------------------------------------------
inline void ArcBall::beginDrag(const Eigen::Vector2f& msc)
{
  // The next two lines are usually a part of end drag. But end drag introduces
  // too much statefullness, so we are shortcircuiting it.
  mQDown      = mQNow;

  // Normal 'begin' code.
  mVDown      = (mScreenToTCS * Eigen::Vector4f(msc.x() , msc.y() , 0.0f, 1.0)).segment<3>(0);
}

//------------------------------------------------------------------------------
inline void ArcBall::drag(const Eigen::Vector2f& msc)
{
  // Regular drag code to follow...
  mVNow       = (mScreenToTCS * Eigen::Vector4f(msc.x() , msc.y() , 0.0, 1.0)).segment<3>(0);
  mVSphereFrom= mouseOnSphere(mVDown);
  mVSphereTo  = mouseOnSphere(mVNow);

  // Construct a quaternion from two points on the unit sphere.
  mQDrag.setFromTwoVectors(mVSphereFrom, mVSphereTo); 
  mQNow = mQDrag * mQDown;

  // Perform complex conjugate
  mMatNow.setIdentity();
  mMatNow.block<3,3>(0,0) = mQNow.inverse().matrix();
}

template<class W>
inline void ArcBall::attach(W window)
{

  // move this setup
  window->movefx = [this] (GLFWwindow *w,double x, double y) 
  {
    if(glfwGetMouseButton(w,0) == GLFW_PRESS)
    {
      this->drag(Eigen::Vector2f(x,y));
    }
  };
  window->mousefx = [this] (GLFWwindow *w,int button, int action, int mods) 
  {
    if(button == 0 && action == GLFW_PRESS)
    {
      double p[2];
      glfwGetCursorPos(w,&p[0],&p[1]); // make helper
      this->beginDrag(Eigen::Vector2f(p[0],p[1]));
    }   
  };  
}

} // namespace CPM_ARC_BALL_NS 

