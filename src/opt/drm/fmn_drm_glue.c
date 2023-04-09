/* akdrm_glue.c
 * Connects the pre-written 'akdrm' unit to Full Moon's 'bigpc' unit.
 */

#include "fmn_drm_internal.h" 
#include "opt/bigpc/bigpc_video.h"
#include "opt/bigpc/bigpc_image.h"
#include <stdio.h>

struct bigpc_video_driver_drm {
  struct bigpc_video_driver hdr;
};

#define DRIVER ((struct bigpc_video_driver_drm*)driver)

static void _drm_del(struct bigpc_video_driver *driver) {
  fmn_drm_quit();
}

static int _drm_init(struct bigpc_video_driver *driver,const struct bigpc_video_config *config) {

  if (fmn_drm_init()<0) {
    fprintf(stderr,"fmn_drm_init failed\n");
    return -1;
  }
  
  driver->renderer=BIGPC_RENDERER_opengl2;
  driver->w=fmn_drm.w;
  driver->h=fmn_drm.h;

  return 0;
}

static int _drm_begin_gx(struct bigpc_video_driver *driver) {
}

static void _drm_end(struct bigpc_video_driver *driver) {
  fmn_drm_swap();
}

static void _drm_cancel(struct bigpc_video_driver *driver) {
}

const struct bigpc_video_type bigpc_video_type_drm={
  .name="drm",
  .desc="Linux Direct Rendering Manager, for systems without an X server.",
  .objlen=sizeof(struct bigpc_video_driver_drm),
  .appointment_only=0,
  .has_wm=0,
  .del=_drm_del,
  .init=_drm_init,
  .begin_gx=_drm_begin_gx,
  .end=_drm_end,
  .cancel=_drm_cancel,
};
