#include "akdrm_internal.h"

/* Init GX, main entry point.
 */
 
int akdrm_init_gx(struct akdrm *akdrm) {
  fprintf(stderr,"TODO %s\n",__func__);
  
  const EGLint context_attribs[]={
    EGL_CONTEXT_CLIENT_VERSION,2,
    EGL_NONE,
  };
  
  const EGLint config_attribs[]={
    EGL_SURFACE_TYPE,EGL_WINDOW_BIT,
    EGL_RED_SIZE,8,
    EGL_GREEN_SIZE,8,
    EGL_BLUE_SIZE,8,
    EGL_ALPHA_SIZE,8,
    EGL_RENDERABLE_TYPE,EGL_OPENGL_BIT,
    EGL_NONE,
  };
  
  const EGLint surface_attribs[]={
    EGL_WIDTH,akdrm->w,
    EGL_HEIGHT,akdrm->h,
    EGL_NONE,
  };
  
  if (!(akdrm->gbmdevice=gbm_create_device(akdrm->fd))) return -1;
  
  if (!(akdrm->gbmsurface=gbm_surface_create(
    akdrm->gbmdevice,
    akdrm->w,akdrm->h,
    GBM_FORMAT_XRGB8888,
    GBM_BO_USE_SCANOUT|GBM_BO_USE_RENDERING
  ))) return -1;
  
  PFNEGLGETPLATFORMDISPLAYEXTPROC eglGetPlatformDisplayEXT=(void*)eglGetProcAddress("eglGetPlatformDisplayEXT");
  if (!eglGetPlatformDisplayEXT) return -1;
  
  if (!(akdrm->egldisplay=eglGetPlatformDisplayEXT(
    EGL_PLATFORM_GBM_KHR,akdrm->gbmdevice,0
  ))) return -1;
  
  EGLint major=0,minor=0;
  if (!eglInitialize(akdrm->egldisplay,&major,&minor)) return -1;
  
  if (!eglBindAPI(EGL_OPENGL_API)) return -1;
  //if (!eglBindAPI(EGL_OPENGL_ES_API)) return -1;
  
  EGLConfig eglconfig;
  EGLint n;
  if (!eglChooseConfig(
    akdrm->egldisplay,config_attribs,&eglconfig,1,&n
  )||(n!=1)) return -1;
  
  if (!(akdrm->eglcontext=eglCreateContext(
    akdrm->egldisplay,eglconfig,EGL_NO_CONTEXT,context_attribs
  ))) return -1;
  
  if (!(akdrm->eglsurface=eglCreateWindowSurface(
    akdrm->egldisplay,eglconfig,akdrm->gbmsurface,0
  ))) return -1;
  
  return 0;
}

/* Begin frame.
 */
 
int akdrm_gx_begin(struct akdrm *akdrm) {
  eglMakeCurrent(akdrm->egldisplay,akdrm->eglsurface,akdrm->eglsurface,akdrm->eglcontext);
  return 0;
}

/* End frame.
 */
 
void akdrm_gx_end(struct akdrm *akdrm) {
  //TODO
}

/* Render framebuffer.
 */
 
void akdrm_gx_render_fb(struct akdrm *akdrm) {
  //TODO
}
