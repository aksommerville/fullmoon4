#include "fmn_glx_internal.h"

/* Initialize display and atoms.
 */
 
static int fmn_glx_init_display(struct fmn_glx *fmn_glx,const struct fmn_glx_setup *setup) {
  
  if (!(fmn_glx->dpy=XOpenDisplay(setup->display))) return -1;
  fmn_glx->screen=DefaultScreen(fmn_glx->dpy);

  #define GETATOM(tag) fmn_glx->atom_##tag=XInternAtom(fmn_glx->dpy,#tag,0);
  GETATOM(WM_PROTOCOLS)
  GETATOM(WM_DELETE_WINDOW)
  GETATOM(_NET_WM_STATE)
  GETATOM(_NET_WM_STATE_FULLSCREEN)
  GETATOM(_NET_WM_STATE_ADD)
  GETATOM(_NET_WM_STATE_REMOVE)
  GETATOM(_NET_WM_ICON)
  GETATOM(_NET_WM_ICON_NAME)
  GETATOM(_NET_WM_NAME)
  GETATOM(STRING)
  GETATOM(UTF8_STRING)
  GETATOM(WM_CLASS)
  #undef GETATOM
  
  return 0;
}

/* Get the size of the monitor we're going to display on.
 * NOT the size of the logical desktop, if it can be avoided.
 * We don't actually know which monitor will be chosen, and we don't want to force it, so use the smallest.
 */
 
static void fmn_glx_estimate_monitor_size(int *w,int *h,const struct fmn_glx *fmn_glx) {
  *w=*h=0;
  #if FMN_USE_xinerama
    int infoc=0;
    XineramaScreenInfo *infov=XineramaQueryScreens(fmn_glx->dpy,&infoc);
    if (infov) {
      if (infoc>0) {
        *w=infov[0].width;
        *h=infov[0].height;
        int i=infoc; while (i-->1) {
          if ((infov[i].width<*w)||(infov[i].height<*h)) {
            *w=infov[i].width;
            *h=infov[i].height;
          }
        }
      }
      XFree(infov);
    }
  #endif
  if ((*w<1)||(*h<1)) {
    *w=DisplayWidth(fmn_glx->dpy,0);
    *h=DisplayHeight(fmn_glx->dpy,0);
    if ((*w<1)||(*h<1)) {
      *w=640;
      *h=480;
    }
  }
}

static void fmn_glx_size_window_for_framebuffer(
  struct fmn_glx *fmn_glx,int fbw,int fbh,int monw,int monh
) {
  // No particular reason for this, but take 3/4 of the monitor as our upper limit.
  // I figure it's polite to leave some breathing room on the user's screen, when not in fullscreen mode.
  int wlimit=(monw*3)>>2;
  int hlimit=(monh*3)>>2;
  // Within that, take some integer multiple of the framebuffer size.
  int xscale=wlimit/fbw;
  int yscale=hlimit/fbh;
  int scale=(xscale<yscale)?xscale:yscale;
  if (scale<1) scale=1;
  fmn_glx->w=fbw*scale;
  fmn_glx->h=fbh*scale;
}

static void fmn_glx_size_window_for_monitor(
  struct fmn_glx *fmn_glx,int monw,int monh
) {
  fmn_glx->w=(monw*3)>>2;
  fmn_glx->h=(monh*3)>>2;
}

/* Init, first stage.
 */
 
int fmn_glx_init_start(struct fmn_glx *fmn_glx,const struct fmn_glx_setup *setup) {

  if (fmn_glx_init_display(fmn_glx,setup)<0) return -1;
  fmn_glx->dstdirty=1;
  
  // Framebuffer size straight from the caller, and force it positive or zero.
  fmn_glx->fbw=setup->fbw;
  fmn_glx->fbh=setup->fbh;
  if ((fmn_glx->fbw<1)||(fmn_glx->fbh<1)) {
    fmn_glx->fbw=0;
    fmn_glx->fbh=0;
  }
  
  // Caller usually doesn't specify a window size, and we default based on frambuffer and monitor.
  fmn_glx->w=setup->w;
  fmn_glx->h=setup->h;
  int monw=0,monh=0;
  if ((fmn_glx->w<1)||(fmn_glx->h<1)) {
    fmn_glx_estimate_monitor_size(&monw,&monh,fmn_glx);
    if (fmn_glx->fbw) fmn_glx_size_window_for_framebuffer(fmn_glx,fmn_glx->fbw,fmn_glx->fbh,monw,monh);
    else fmn_glx_size_window_for_monitor(fmn_glx,monw,monh);
  }
  
  // video_mode defaults based on whether a framebuffer is requested.
  fmn_glx->video_mode=setup->video_mode;
  if (fmn_glx->video_mode==FMN_GLX_VIDEO_MODE_AUTO) {
    // We don't ever recommend FB_PURE; client has to ask for it specifically.
    if (fmn_glx->fbw) {
      fmn_glx->video_mode=FMN_GLX_VIDEO_MODE_FB_GX;
    } else {
      fmn_glx->video_mode=FMN_GLX_VIDEO_MODE_OPENGL;
    }
  }
  
  return 0;
}

/* Initialize window with GX.
 */
 
static GLXFBConfig fmn_glx_get_fbconfig(struct fmn_glx *fmn_glx) {

  int attrv[]={
    GLX_X_RENDERABLE,1,
    GLX_DRAWABLE_TYPE,GLX_WINDOW_BIT,
    GLX_RENDER_TYPE,GLX_RGBA_BIT,
    GLX_X_VISUAL_TYPE,GLX_TRUE_COLOR,
    GLX_RED_SIZE,8,
    GLX_GREEN_SIZE,8,
    GLX_BLUE_SIZE,8,
    GLX_ALPHA_SIZE,0,
    GLX_DEPTH_SIZE,0,
    GLX_STENCIL_SIZE,0,
    GLX_DOUBLEBUFFER,1,
  0};
  
  if (!glXQueryVersion(fmn_glx->dpy,&fmn_glx->glx_version_major,&fmn_glx->glx_version_minor)) {
    return (GLXFBConfig){0};
  }
  
  int fbc=0;
  GLXFBConfig *configv=glXChooseFBConfig(fmn_glx->dpy,fmn_glx->screen,attrv,&fbc);
  if (!configv||(fbc<1)) return (GLXFBConfig){0};
  GLXFBConfig config=configv[0];
  XFree(configv);
  
  return config;
}
 
static int fmn_glx_init_window_gx_inner(struct fmn_glx *fmn_glx,const struct fmn_glx_setup *setup,XVisualInfo *vi) {
  
  // Ask for mouse events only if implemented by the delegate. Otherwise they're just noise.
  XSetWindowAttributes wattr={
    .event_mask=
      StructureNotifyMask|
      KeyPressMask|KeyReleaseMask|
      FocusChangeMask|
    0,
  };
  if (fmn_glx->delegate.mmotion) {
    wattr.event_mask|=PointerMotionMask;
  }
  if (fmn_glx->delegate.mbutton||fmn_glx->delegate.mwheel) {
    wattr.event_mask|=ButtonPressMask|ButtonReleaseMask;
  }
  wattr.colormap=XCreateColormap(fmn_glx->dpy,RootWindow(fmn_glx->dpy,vi->screen),vi->visual,AllocNone);
  
  if (!(fmn_glx->win=XCreateWindow(
    fmn_glx->dpy,RootWindow(fmn_glx->dpy,fmn_glx->screen),
    0,0,fmn_glx->w,fmn_glx->h,0,
    vi->depth,InputOutput,vi->visual,
    CWBorderPixel|CWColormap|CWEventMask,&wattr
  ))) return -1;
  
  return 0;
}
 
static int fmn_glx_init_window_gx(struct fmn_glx *fmn_glx,const struct fmn_glx_setup *setup) {
  GLXFBConfig fbconfig=fmn_glx_get_fbconfig(fmn_glx);
  XVisualInfo *vi=glXGetVisualFromFBConfig(fmn_glx->dpy,fbconfig);
  if (!vi) return -1;
  int err=fmn_glx_init_window_gx_inner(fmn_glx,setup,vi);
  XFree(vi);
  if (err<0) return -1;
  if (!(fmn_glx->ctx=glXCreateNewContext(fmn_glx->dpy,fbconfig,GLX_RGBA_TYPE,0,1))) return -1;
  glXMakeCurrent(fmn_glx->dpy,fmn_glx->win,fmn_glx->ctx);
  return 0;
}

/* Init public OpenGL context.
 */
 
int fmn_glx_init_opengl(struct fmn_glx *fmn_glx,const struct fmn_glx_setup *setup) {
  if (fmn_glx_init_window_gx(fmn_glx,setup)<0) return -1;
  return 0;
}

/* Init with OpenGL but a framebuffer to the client.
 */
 
int fmn_glx_init_fb_gx(struct fmn_glx *fmn_glx,const struct fmn_glx_setup *setup) {
  if (fmn_glx_init_window_gx(fmn_glx,setup)<0) return -1;
  
  // Allocate OpenGL texture.
  glGenTextures(1,&fmn_glx->fbtexid);
  if (!fmn_glx->fbtexid) {
    glGenTextures(1,&fmn_glx->fbtexid);
    if (!fmn_glx->fbtexid) return -1;
  }
  glBindTexture(GL_TEXTURE_2D,fmn_glx->fbtexid);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
  
  // Allocate framebuffer for exposure to client.
  // We have a strong bias for RGBX, since that is a well-supported OpenGL format.
  fmn_glx->fbfmt=setup->fbfmt;
  if (fmn_glx->fbfmt==FMN_GLX_FBFMT_ANYTHING) {
    #if BYTE_ORDER==LITTLE_ENDIAN
      fmn_glx->fbfmt=FMN_GLX_FBFMT_XBGR;
    #else
      fmn_glx->fbfmt=FMN_GLX_FBFMT_RGBX;
    #endif
  }
  int buffersize=fmn_glx_fbfmt_measure_buffer(fmn_glx->fbfmt,fmn_glx->fbw,fmn_glx->fbh);
  if (buffersize<1) return -1;
  if (!(fmn_glx->fb=malloc(buffersize))) return -1;
  
  // If the client framebuffer is not RGBX, allocate a conversion buffer.
  // There are other formats we could upload directly, but I think it won't be very common, and safer to just convert them.
  int isrgbx=0;
  #if BYTE_ORDER==LITTLE_ENDIAN
    if (fmn_glx->fbfmt==FMN_GLX_FBFMT_XBGR) isrgbx=1;
  #else
    if (fmn_glx->fbfmt==FMN_GLX_FBFMT_RGBX) isrgbx=1;
  #endif
  if (!isrgbx) {
    int cvtsize=fmn_glx->fbw*fmn_glx->fbh*4;
    if (!(fmn_glx->fbcvt=malloc(cvtsize))) return -1;
  }
  
  return 0;
}

/* Init public framebuffer with software scaling.
 */
 
static int fmn_glx_init_window_fb_inner(struct fmn_glx *fmn_glx,const struct fmn_glx_setup *setup) {
  
  // Ask for mouse events only if implemented by the delegate. Otherwise they're just noise.
  XSetWindowAttributes wattr={
    .event_mask=
      StructureNotifyMask|
      KeyPressMask|KeyReleaseMask|
      FocusChangeMask|
    0,
  };
  if (fmn_glx->delegate.mmotion) {
    wattr.event_mask|=PointerMotionMask;
  }
  if (fmn_glx->delegate.mbutton||fmn_glx->delegate.mwheel) {
    wattr.event_mask|=ButtonPressMask|ButtonReleaseMask;
  }
  
  if (!(fmn_glx->win=XCreateWindow(
    fmn_glx->dpy,RootWindow(fmn_glx->dpy,fmn_glx->screen),
    0,0,fmn_glx->w,fmn_glx->h,0,
    DefaultDepth(fmn_glx->dpy,fmn_glx->screen),InputOutput,CopyFromParent,
    CWBorderPixel|CWEventMask,&wattr
  ))) return -1;
  
  return 0;
}
 
int fmn_glx_init_fb_pure(struct fmn_glx *fmn_glx,const struct fmn_glx_setup *setup) {

  fmn_glx->scale_limit=setup->scale_limit;
  
  if (fmn_glx_init_window_fb_inner(fmn_glx,setup)<0) return -1;
  
  if (!(fmn_glx->gc=XCreateGC(fmn_glx->dpy,fmn_glx->win,0,0))) return -1;
  
  // Allocate framebuffer for exposure to client.
  // Prefer the X display's format if client didn't request one.
  fmn_glx->fbfmt=setup->fbfmt;
  if (fmn_glx->fbfmt==FMN_GLX_FBFMT_ANYTHING) {
    Visual *visual=DefaultVisual(fmn_glx->dpy,fmn_glx->screen);
         if ((visual->red_mask==0x000000ff)&&(visual->green_mask==0x0000ff00)&&(visual->blue_mask==0x00ff0000)) fmn_glx->fbfmt=FMN_GLX_FBFMT_XBGR;
    else if ((visual->red_mask==0x00ff0000)&&(visual->green_mask==0x0000ff00)&&(visual->blue_mask==0x000000ff)) fmn_glx->fbfmt=FMN_GLX_FBFMT_XRGB;
    else if ((visual->red_mask==0xff000000)&&(visual->green_mask==0x00ff0000)&&(visual->blue_mask==0x0000ff00)) fmn_glx->fbfmt=FMN_GLX_FBFMT_RGBX;
    else if ((visual->red_mask==0x0000ff00)&&(visual->green_mask==0x00ff0000)&&(visual->blue_mask==0xff000000)) fmn_glx->fbfmt=FMN_GLX_FBFMT_BGRX;
    else fmn_glx->fbfmt=FMN_GLX_FBFMT_XRGB;
  }
  int buffersize=fmn_glx_fbfmt_measure_buffer(fmn_glx->fbfmt,fmn_glx->fbw,fmn_glx->fbh);
  if (buffersize<1) return -1;
  if (!(fmn_glx->fb=malloc(buffersize))) return -1;

  return 0;
}

/* Set window title.
 */
 
static void fmn_glx_set_title(struct fmn_glx *fmn_glx,const char *src) {
  
  // I've seen these properties in GNOME 2, unclear whether they might still be in play:
  XTextProperty prop={.value=(void*)src,.encoding=fmn_glx->atom_STRING,.format=8,.nitems=0};
  while (prop.value[prop.nitems]) prop.nitems++;
  XSetWMName(fmn_glx->dpy,fmn_glx->win,&prop);
  XSetWMIconName(fmn_glx->dpy,fmn_glx->win,&prop);
  XSetTextProperty(fmn_glx->dpy,fmn_glx->win,&prop,fmn_glx->atom__NET_WM_ICON_NAME);
    
  // This one becomes the window title and bottom-bar label, in GNOME 3:
  prop.encoding=fmn_glx->atom_UTF8_STRING;
  XSetTextProperty(fmn_glx->dpy,fmn_glx->win,&prop,fmn_glx->atom__NET_WM_NAME);
    
  // This daffy bullshit becomes the Alt-Tab text in GNOME 3:
  {
    char tmp[256];
    int len=prop.nitems+1+prop.nitems;
    if (len<sizeof(tmp)) {
      memcpy(tmp,prop.value,prop.nitems);
      tmp[prop.nitems]=0;
      memcpy(tmp+prop.nitems+1,prop.value,prop.nitems);
      tmp[prop.nitems+1+prop.nitems]=0;
      prop.value=tmp;
      prop.nitems=prop.nitems+1+prop.nitems;
      prop.encoding=fmn_glx->atom_STRING;
      XSetTextProperty(fmn_glx->dpy,fmn_glx->win,&prop,fmn_glx->atom_WM_CLASS);
    }
  }
}

/* Set window icon.
 */
 
static void fmn_glx_copy_icon_pixels(long *dst,const uint8_t *src,int c) {
  for (;c-->0;dst++,src+=4) {
    *dst=(src[3]<<24)|(src[0]<<16)|(src[1]<<8)|src[2];
  }
}
 
static void fmn_glx_set_icon(struct fmn_glx *fmn_glx,const void *rgba,int w,int h) {
  if (!rgba||(w<1)||(h<1)||(w>FMN_GLX_ICON_SIZE_LIMIT)||(h>FMN_GLX_ICON_SIZE_LIMIT)) return;
  int length=2+w*h;
  long *pixels=malloc(sizeof(long)*length);
  if (!pixels) return;
  pixels[0]=w;
  pixels[1]=h;
  fmn_glx_copy_icon_pixels(pixels+2,rgba,w*h);
  int result=XChangeProperty(fmn_glx->dpy,fmn_glx->win,fmn_glx->atom__NET_WM_ICON,XA_CARDINAL,32,PropModeReplace,(unsigned char*)pixels,length);
  free(pixels);
}

/* Init, last stage.
 */

int fmn_glx_init_finish(struct fmn_glx *fmn_glx,const struct fmn_glx_setup *setup) {

  if (setup->title&&setup->title[0]) {
    fmn_glx_set_title(fmn_glx,setup->title);
  }
  
  if (setup->iconrgba&&(setup->iconw>0)&&(setup->iconh>0)) {
    fmn_glx_set_icon(fmn_glx,setup->iconrgba,setup->iconw,setup->iconh);
  }
  
  if (setup->fullscreen) {
    XChangeProperty(
      fmn_glx->dpy,fmn_glx->win,
      fmn_glx->atom__NET_WM_STATE,
      XA_ATOM,32,PropModeReplace,
      (unsigned char*)&fmn_glx->atom__NET_WM_STATE_FULLSCREEN,1
    );
    fmn_glx->fullscreen=1;
  }
  
  XMapWindow(fmn_glx->dpy,fmn_glx->win);
  XSync(fmn_glx->dpy,0);
  XSetWMProtocols(fmn_glx->dpy,fmn_glx->win,&fmn_glx->atom_WM_DELETE_WINDOW,1);
  
  fmn_glx->cursor_visible=1; // x11's default...
  fmn_glx_show_cursor(fmn_glx,0); // our default

  return 0;
}
