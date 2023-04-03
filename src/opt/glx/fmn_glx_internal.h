#ifndef FMN_GLX_INTERNAL_H
#define FMN_GLX_INTERNAL_H

#include "fmn_glx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <endian.h>
#include <limits.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/XKBlib.h>
#include <X11/keysym.h>
#include <GL/glx.h>
#include <GL/gl.h>

// Required only for making intelligent initial-size decisions in a multi-monitor setting.
// apt install libxinerama-dev
#ifndef FMN_USE_xinerama
  #define FMN_USE_xinerama 1
#endif
#if FMN_USE_xinerama
  #include <X11/extensions/Xinerama.h>
#endif

#define KeyRepeat (LASTEvent+2)
#define FMN_GLX_KEY_REPEAT_INTERVAL 10
#define FMN_GLX_ICON_SIZE_LIMIT 64

struct fmn_glx {
  struct fmn_glx_delegate delegate;
  
  Display *dpy;
  int screen;
  Window win;
  GC gc;
  int w,h;
  int fullscreen;
  int video_mode;
  int fbfmt;

  GLXContext ctx;
  int glx_version_minor,glx_version_major;
  
  Atom atom_WM_PROTOCOLS;
  Atom atom_WM_DELETE_WINDOW;
  Atom atom__NET_WM_STATE;
  Atom atom__NET_WM_STATE_FULLSCREEN;
  Atom atom__NET_WM_STATE_ADD;
  Atom atom__NET_WM_STATE_REMOVE;
  Atom atom__NET_WM_ICON;
  Atom atom__NET_WM_ICON_NAME;
  Atom atom__NET_WM_NAME;
  Atom atom_WM_CLASS;
  Atom atom_STRING;
  Atom atom_UTF8_STRING;
  
  int screensaver_suppressed;
  int focus;
  int cursor_visible;
  
  // Used by typical GX and FB modes.
  GLuint fbtexid;
  uint8_t *fb;
  int fbw,fbh;
  int dstdirty; // Nonzero to recalculate bounds next render.
  int dstx,dsty,dstw,dsth;
  
  // Conversion buffer for FMN_GLX_VIDEO_MODE_FB_GX. Always serial RGBX, same dimensions as (fb).
  void *fbcvt;
  
  // Scale-up image, used by FMN_GLX_VIDEO_MODE_FB_PURE.
  XImage *image;
  int rshift,gshift,bshift;
  int scale_reformat; // if zero, (image) happens to be the public framebuffer format.
  int scale;
  int scale_limit;
};

/* Start establishes (video_mode,dpy,screen,atom_*).
 * The mode-specific initializers, call just one, make (win,fb,image), whatever it needs.
 * Finish sets window properties. Title, cursor, etc.
 */
int fmn_glx_init_start(struct fmn_glx *fmn_glx,const struct fmn_glx_setup *setup);
int fmn_glx_init_opengl(struct fmn_glx *fmn_glx,const struct fmn_glx_setup *setup);
int fmn_glx_init_fb_pure(struct fmn_glx *fmn_glx,const struct fmn_glx_setup *setup);
int fmn_glx_init_fb_gx(struct fmn_glx *fmn_glx,const struct fmn_glx_setup *setup);
int fmn_glx_init_finish(struct fmn_glx *fmn_glx,const struct fmn_glx_setup *setup);

// To serial RGBX for OpenGL.
void fmn_glx_fbcvt_rgb(void *dst,uint8_t drs,uint8_t dgs,uint8_t dbs,const void *src,uint8_t srs,uint8_t sgs,uint8_t sbs,int w,int h);
void fmn_glx_fbcvt_rgb_bgr565le(void *dst,const void *src,int w,int h);
void fmn_glx_fbcvt_rgb_bgrx4444be(void *dst,const void *src,int w,int h);
void fmn_glx_fbcvt_rgb_bgr332(void *dst,const void *src,int w,int h);
void fmn_glx_fbcvt_rgb_y1(void *dst,const void *src,int w,int h);
void fmn_glx_fbcvt_rgb_y8(void *dst,const void *src,int w,int h);

// To native RGBX for X11.
void fmn_glx_scale_swizzle(void *dst,const void *src,int w,int h,int scale,uint8_t drs,uint8_t dgs,uint8_t dbs,uint8_t srs,uint8_t sgs,uint8_t sbs);
void fmn_glx_scale_bgr565le(void *dst,const void *src,int w,int h,int scale,uint8_t drs,uint8_t dgs,uint8_t dbs);
void fmn_glx_scale_bgrx4444be(void *dst,const void *src,int w,int h,int scale,uint8_t drs,uint8_t dgs,uint8_t dbs);
void fmn_glx_scale_bgr332(void *dst,const void *src,int w,int h,int scale,uint8_t drs,uint8_t dgs,uint8_t dbs);
void fmn_glx_scale_y1(void *dst,const void *src,int w,int h,int scale);
void fmn_glx_scale_y8(void *dst,const void *src,int w,int h,int scale);
void fmn_glx_scale_same32(void *dst,const void *src,int w,int h,int scale);

#endif
