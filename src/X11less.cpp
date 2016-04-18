// See http://arrayfire.com/remote-off-screen-rendering-with-opengl/
// 
#define USE_EGL_GET_DISPLAY
#define USE_EGL_SURFACE

#ifdef USE_EGL_GET_DISPLAY
#include <EGL/egl.h>
#else
#define EGL_EGLEXT_PROTOTYPES
#include <EGL/egl.h>
#include <EGL/eglext.h>
#endif
#include <stdio.h>
  static const EGLint configAttribs[] = {
          EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
          EGL_BLUE_SIZE, 8,
          EGL_GREEN_SIZE, 8,
          EGL_RED_SIZE, 8,
          EGL_DEPTH_SIZE,EGL_DONT_CARE,
	  EGL_ALPHA_SIZE,EGL_DONT_CARE, 
          EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
          EGL_NONE
  };    


  static const int pbufferWidth = 9;
  static const int pbufferHeight = 9;

  static const EGLint pbufferAttribs[] = {
        EGL_WIDTH, pbufferWidth,
        EGL_HEIGHT, pbufferHeight,
        EGL_NONE,
  };

int main(int argc, char *argv[])
{
  // 1. Initialize EGL
#ifdef USE_EGL_GET_DISPLAY
    // obtain the display via default display. 
    EGLDisplay eglDpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
#else
    // obtain display by specifying an appropriate device. This is the preferred method today.

    // load the function pointers for the device,platform extensions 
    PFNEGLQUERYDEVICESEXTPROC eglQueryDevicesEXT =
               (PFNEGLQUERYDEVICESEXTPROC) eglGetProcAddress("eglQueryDevicesEXT");
    if(!eglQueryDevicesEXT) { 
         printf("ERROR: extension eglQueryDevicesEXT not available"); 
         return(-1); 
    } 
    
    PFNEGLGETPLATFORMDISPLAYEXTPROC eglGetPlatformDisplayEXT =
               (PFNEGLGETPLATFORMDISPLAYEXTPROC)eglGetProcAddress("eglGetPlatformDisplayEXT");
    if(!eglGetPlatformDisplayEXT) { 
         printf("ERROR: extension eglGetPlatformDisplayEXT not available"); 
         return(-1);  
    }
  
    static const int MAX_DEVICES = 16;
    EGLDeviceEXT devices[MAX_DEVICES];
    EGLint numDevices;

    eglQueryDevicesEXT(MAX_DEVICES, devices, &numDevices);
printf("Devices %d with device %p\n",numDevices,devices[0]);
//eglQueryDeviceAttribEXT(devices[0],
    EGLDisplay eglDpy = eglGetPlatformDisplayEXT(EGL_PLATFORM_DEVICE_EXT, devices[0], 0);
#endif    

printf("Display from eglGetPlatformDisplay is %p\n",eglDpy);
  EGLint major=0, minor=0;

  int r = eglInitialize(eglDpy, &major, &minor);

printf("eglInitialize result %d Found %d %d Error %X\n",r,major,minor,eglGetError());

  // 2. Select an appropriate configuration
  EGLint numConfigs;
  EGLConfig eglCfg;

  eglChooseConfig(eglDpy, configAttribs, &eglCfg, 1, &numConfigs);
printf("Available configs %d\n",numConfigs);
  // 3. Create a surface
  
 // 3. Bind the API 
    eglBindAPI(EGL_OPENGL_API);

    // 4. create the context
    EGLContext eglCtx = eglCreateContext(eglDpy, eglCfg, EGL_NO_CONTEXT, NULL);

EGLSurface eglSurf = eglCreatePbufferSurface(eglDpy, eglCfg, 
                                               pbufferAttribs);
printf("Surface is %p\n",eglSurf);

#ifdef USE_EGL_SURFACE
    // 5. create the surface and make the context current current
    eglMakeCurrent(eglDpy, eglSurf, eglSurf, eglCtx);
#else
    // 5. make the context current without a surface
    eglMakeCurrent(eglDpy, EGL_NO_SURFACE, EGL_NO_SURFACE, eglCtx);
#endif

  


// from now on use your OpenGL context

  // 6. Terminate EGL when finished
  eglTerminate(eglDpy);
  return 0;
}
