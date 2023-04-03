/* akdrm_glue.c
 * Connects the pre-written 'akdrm' unit to Full Moon's 'bigpc' unit.
 */
 
#include "akdrm.h"
#include "opt/bigpc/bigpc_video.h"
#include "opt/bigpc/bigpc_image.h"

struct bigpc_video_driver_drm {
  struct bigpc_video_driver hdr;
  struct akdrm *akdrm;
  struct bigpc_image *fb; // null if GX
};

#define DRIVER ((struct bigpc_video_driver_drm*)driver)

static void _drm_del(struct bigpc_video_driver *driver) {
  akdrm_del(DRIVER->akdrm);
  bigpc_image_del(DRIVER->fb);
}

static int _drm_init(struct bigpc_video_driver *driver,const struct bigpc_video_config *config) {

  struct akdrm_setup setup={
    .video_mode=0,
    .fbfmt=0,
    .fbw=config->fbw,
    .fbh=config->fbh,
  };
  switch (config->renderer) {
    case BIGPC_RENDERER_any: driver->renderer=BIGPC_RENDERER_opengl2; setup.video_mode=AKDRM_VIDEO_MODE_AUTO; break;
    case BIGPC_RENDERER_soft: driver->renderer=BIGPC_RENDERER_rgb32; setup.video_mode=AKDRM_VIDEO_MODE_FB_PURE; setup.fbfmt=AKDRM_FBFMT_ANYTHING; break;
    case BIGPC_RENDERER_gx: driver->renderer=BIGPC_RENDERER_opengl2; setup.video_mode=AKDRM_VIDEO_MODE_OPENGL; break;
    // We'll assume that "ANYTHING" will give us 32-bit RGB as we promise. We could select one of the explicit RGBs, but might guess wrong.
    case BIGPC_RENDERER_rgb32: driver->renderer=BIGPC_RENDERER_rgb32; setup.video_mode=AKDRM_VIDEO_MODE_FB_PURE; setup.fbfmt=AKDRM_FBFMT_ANYTHING; break;
    case BIGPC_RENDERER_opengl1: driver->renderer=BIGPC_RENDERER_opengl1; setup.video_mode=AKDRM_VIDEO_MODE_OPENGL; break;
    case BIGPC_RENDERER_opengl2: driver->renderer=BIGPC_RENDERER_opengl2; setup.video_mode=AKDRM_VIDEO_MODE_OPENGL; break;
    default: return -1;
  }

  struct akdrm_config akdrm_config={0};
  if (akdrm_find_device(&akdrm_config,0,60,config->w,config->h,config->fbw,config->fbh)<0) return -1;
  
  DRIVER->akdrm=akdrm_new(&akdrm_config,&setup);
  akdrm_config_cleanup(&akdrm_config);
  if (!DRIVER->akdrm) return -1;
  
  if (akdrm_video_mode_is_fb(akdrm_get_video_mode(DRIVER->akdrm))) {
    uint8_t pixels[]={1,2,3,4}; // dummy just to create the image; we'll replace at each frame.
    int fbw=0,fbh=0;
    akdrm_get_fb_size(&fbw,&fbh,DRIVER->akdrm);
    int fbfmt=akdrm_get_fbfmt(DRIVER->akdrm);
    int stride=akdrm_fbfmt_measure_stride(fbfmt,fbw);
    if (stride<fbw<<2) return -1; // oops it can't be 32-bit.
    int storage=BIGPC_IMAGE_STORAGE_32;
    int pixfmt;
    switch (fbfmt) {
      case AKDRM_FBFMT_RGBX: pixfmt=BIGPC_IMAGE_PIXFMT_RGBX; break;
      case AKDRM_FBFMT_BGRX: pixfmt=BIGPC_IMAGE_PIXFMT_BGRX; break;
      case AKDRM_FBFMT_XRGB: pixfmt=BIGPC_IMAGE_PIXFMT_XRGB; break;
      case AKDRM_FBFMT_XBGR: pixfmt=BIGPC_IMAGE_PIXFMT_XBGR; break;
      default: return -1;
    }
    if (!(DRIVER->fb=bigpc_image_new_borrow(pixels,fbw,fbh,stride,storage,pixfmt,0))) return -1;
    DRIVER->fb->flags|=BIGPC_IMAGE_FLAG_WRITEABLE;
  }
  
  akdrm_get_size(&driver->w,&driver->h,DRIVER->akdrm);

  return 0;
}

static struct bigpc_image *_drm_begin_soft(struct bigpc_video_driver *driver) {
  if (!DRIVER->fb) return 0;
  if (!(DRIVER->fb->v=akdrm_begin_fb(DRIVER->akdrm))) return 0;
  return DRIVER->fb;
}

static int _drm_begin_gx(struct bigpc_video_driver *driver) {
  if (DRIVER->fb) return -1;
  return akdrm_begin_gx(DRIVER->akdrm);
}

static void _drm_end(struct bigpc_video_driver *driver) {
  if (DRIVER->fb) akdrm_end_fb(DRIVER->akdrm,DRIVER->fb->v);
  else akdrm_end_gx(DRIVER->akdrm);
}

static void _drm_cancel(struct bigpc_video_driver *driver) {
  // fmn_glx has no concept of cancel (TODO Can we just not 'end' the frame?)
  if (DRIVER->fb) akdrm_end_fb(DRIVER->akdrm,DRIVER->fb->v);
  else akdrm_end_gx(DRIVER->akdrm);
}

const struct bigpc_video_type bigpc_video_type_drm={
  .name="drm",
  .desc="Linux Direct Rendering Manager, for systems without an X server.",
  .objlen=sizeof(struct bigpc_video_driver_drm),
  .appointment_only=0,
  .has_wm=0,
  .del=_drm_del,
  .init=_drm_init,
  .begin_soft=_drm_begin_soft,
  .begin_gx=_drm_begin_gx,
  .end=_drm_end,
  .cancel=_drm_cancel,
};
