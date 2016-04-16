// See http://arrayfire.com/remote-off-screen-rendering-with-opengl/
// 
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
          EGL_DEPTH_SIZE, 8,
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

printf("Display %p\n",eglDpy);
  EGLint major=0, minor=0;

  eglInitialize(eglDpy, &major, &minor);

printf("Found %d %d\n",major,minor);

  // 2. Select an appropriate configuration
  EGLint numConfigs;
  EGLConfig eglCfg;

  eglChooseConfig(eglDpy, configAttribs, &eglCfg, 1, &numConfigs);

  // 3. Create a surface
  EGLSurface eglSurf = eglCreatePbufferSurface(eglDpy, eglCfg, 
                                               pbufferAttribs);

  // 4. Bind the API
  eglBindAPI(EGL_OPENGL_API);

  // 5. Create a context and make it current
  EGLContext eglCtx = eglCreateContext(eglDpy, eglCfg, EGL_NO_CONTEXT,                       NULL);
printf("Context %p\n",eglCtx);
  eglMakeCurrent(eglDpy, eglSurf, eglSurf, eglCtx);

  // from now on use your OpenGL context

  // 6. Terminate EGL when finished
  eglTerminate(eglDpy);
  return 0;
}
