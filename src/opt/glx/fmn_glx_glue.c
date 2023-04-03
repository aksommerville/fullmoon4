/* fmn_glx_glue.c
 * Connects our 'glx' unit to our 'bigpc' unit.
 * 'glx' is a standalone thing that wasn't written specifically for Full Moon.
 */

#include "fmn_glx.h"
#include "opt/bigpc/bigpc_video.h"
#include "opt/bigpc/bigpc_image.h"
#include <stdio.h>

struct bigpc_video_driver_glx {
  struct bigpc_video_driver hdr;
  struct fmn_glx *glx;
  struct bigpc_image *fb; // null if GX
};

#define DRIVER ((struct bigpc_video_driver_glx*)driver)

static void _glx_del(struct bigpc_video_driver *driver) {
  fmn_glx_del(DRIVER->glx);
  bigpc_image_del(DRIVER->fb);
}

static int _glx_init(struct bigpc_video_driver *driver,const struct bigpc_video_config *config) {

  // The fmn_glx and bigpc video delegates are pretty much the same (not a coincidence, written by the same guy).
  struct fmn_glx_delegate delegate={
    .userdata=driver,
    .close=(void*)driver->delegate.cb_close,
    .focus=(void*)driver->delegate.cb_focus,
    .resize=(void*)driver->delegate.cb_resize,
    .key=(void*)driver->delegate.cb_key,
    .text=(void*)driver->delegate.cb_text,
    .mmotion=(void*)driver->delegate.cb_mmotion,
    .mbutton=(void*)driver->delegate.cb_mbutton,
    .mwheel=(void*)driver->delegate.cb_mwheel,
  };
  struct fmn_glx_setup setup={
    .title=config->title,
    .iconrgba=config->iconrgba,
    .iconw=config->iconw,
    .iconh=config->iconh,
    .w=config->w,
    .h=config->h,
    .fbw=config->fbw,
    .fbh=config->fbh,
    .fullscreen=config->fullscreen,
    .video_mode=0, // see below
    .fbfmt=0, // see below
    .scale_limit=0, // TODO configurable?
    .display=0, // TODO configurable?
  };
  
  switch (config->renderer) {
    case BIGPC_RENDERER_any: driver->renderer=BIGPC_RENDERER_opengl2; setup.video_mode=FMN_GLX_VIDEO_MODE_OPENGL; break;
    case BIGPC_RENDERER_soft: driver->renderer=BIGPC_RENDERER_rgb32; setup.video_mode=FMN_GLX_VIDEO_MODE_FB_PURE; setup.fbfmt=FMN_GLX_FBFMT_ANYTHING; break;
    case BIGPC_RENDERER_gx: driver->renderer=BIGPC_RENDERER_opengl2; setup.video_mode=FMN_GLX_VIDEO_MODE_OPENGL; break;
    // We'll assume that "ANYTHING" will give us 32-bit RGB as we promise. We could select one of the explicit RGBs, but might guess wrong.
    case BIGPC_RENDERER_rgb32: driver->renderer=BIGPC_RENDERER_rgb32; setup.video_mode=FMN_GLX_VIDEO_MODE_FB_PURE; setup.fbfmt=FMN_GLX_FBFMT_ANYTHING; break;
    case BIGPC_RENDERER_opengl1: driver->renderer=BIGPC_RENDERER_opengl1; setup.video_mode=FMN_GLX_VIDEO_MODE_OPENGL; break;
    case BIGPC_RENDERER_opengl2: driver->renderer=BIGPC_RENDERER_opengl2; setup.video_mode=FMN_GLX_VIDEO_MODE_OPENGL; break;
    default: return -1;
  }
  
  if (!(DRIVER->glx=fmn_glx_new(&delegate,&setup))) return -1;
  
  if (fmn_glx_video_mode_is_fb(fmn_glx_get_video_mode(DRIVER->glx))) {
    uint8_t pixels[]={1,2,3,4}; // dummy just to create the image; we'll replace at each frame.
    int fbw=0,fbh=0;
    fmn_glx_get_fb_size(&fbw,&fbh,DRIVER->glx);
    int fbfmt=fmn_glx_get_fbfmt(DRIVER->glx);
    int stride=fmn_glx_fbfmt_measure_stride(fbfmt,fbw);
    if (stride<fbw<<2) return -1; // oops it can't be 32-bit.
    int storage=BIGPC_IMAGE_STORAGE_32;
    int pixfmt;
    switch (fbfmt) {
      case FMN_GLX_FBFMT_RGBX: pixfmt=BIGPC_IMAGE_PIXFMT_RGBX; break;
      case FMN_GLX_FBFMT_BGRX: pixfmt=BIGPC_IMAGE_PIXFMT_BGRX; break;
      case FMN_GLX_FBFMT_XRGB: pixfmt=BIGPC_IMAGE_PIXFMT_XRGB; break;
      case FMN_GLX_FBFMT_XBGR: pixfmt=BIGPC_IMAGE_PIXFMT_XBGR; break;
      default: return -1;
    }
    if (!(DRIVER->fb=bigpc_image_new_borrow(pixels,fbw,fbh,stride,storage,pixfmt,0))) return -1;
    DRIVER->fb->flags|=BIGPC_IMAGE_FLAG_WRITEABLE;
  }
  
  fmn_glx_get_size(&driver->w,&driver->h,DRIVER->glx);//TODO we need to stay apprised of these. intercept cb_resize

  return 0;
}

static int _glx_update(struct bigpc_video_driver *driver) {
  return fmn_glx_update(DRIVER->glx);
}

static struct bigpc_image *_glx_begin_soft(struct bigpc_video_driver *driver) {
  if (!DRIVER->fb) return 0;
  if (!(DRIVER->fb->v=fmn_glx_begin_fb(DRIVER->glx))) return 0;
  return DRIVER->fb;
}

static int _glx_begin_gx(struct bigpc_video_driver *driver) {
  if (DRIVER->fb) return -1;
  return fmn_glx_begin_gx(DRIVER->glx);
}

static void _glx_end(struct bigpc_video_driver *driver) {
  if (DRIVER->fb) fmn_glx_end_fb(DRIVER->glx,DRIVER->fb->v);
  else fmn_glx_end_gx(DRIVER->glx);
}

static void _glx_cancel(struct bigpc_video_driver *driver) {
  // fmn_glx has no concept of cancel (TODO Can we just not 'end' the frame?)
  if (DRIVER->fb) fmn_glx_end_fb(DRIVER->glx,DRIVER->fb->v);
  else fmn_glx_end_gx(DRIVER->glx);
}

static void _glx_show_cursor(struct bigpc_video_driver *driver,int show) {
  fmn_glx_show_cursor(DRIVER->glx,show);
}

static void _glx_set_fullscreen(struct bigpc_video_driver *driver,int fullscreen) {
  fmn_glx_set_fullscreen(DRIVER->glx,fullscreen);
}

static void _glx_suppress_screensaver(struct bigpc_video_driver *driver) {
  fmn_glx_suppress_screensaver(DRIVER->glx);
}

const struct bigpc_video_type bigpc_video_type_glx={
  .name="glx",
  .desc="Video and input for Linux via X11 and GLX, prefer for most systems.",
  .objlen=sizeof(struct bigpc_video_driver_glx),
  .appointment_only=0,
  .has_wm=1,
  .del=_glx_del,
  .init=_glx_init,
  .update=_glx_update,
  .begin_soft=_glx_begin_soft,
  .begin_gx=_glx_begin_gx,
  .end=_glx_end,
  .cancel=_glx_cancel,
  .show_cursor=_glx_show_cursor,
  .set_fullscreen=_glx_set_fullscreen,
  .suppress_screensaver=_glx_suppress_screensaver,
};
