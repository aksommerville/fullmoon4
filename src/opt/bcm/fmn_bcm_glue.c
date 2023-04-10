#include "fmn_bcm.h"
#include "opt/bigpc/bigpc_video.h"
#include <stdio.h>

struct bigpc_video_driver_bcm {
  struct bigpc_video_driver hdr;
};

#define DRIVER ((struct bigpc_video_driver_bcm*)driver)

static void _bcm_del(struct bigpc_video_driver *driver) {
  fmn_bcm_quit();
}

static int _bcm_init(struct bigpc_video_driver *driver,const struct bigpc_video_config *config) {
  if (fmn_bcm_init()<0) return -1;
  driver->renderer=BIGPC_RENDERER_opengl2;
  driver->w=fmn_bcm_get_width();
  driver->h=fmn_bcm_get_height();
  return 0;
}

static int _bcm_begin_gx(struct bigpc_video_driver *driver) {
  return 0;
}

static void _bcm_end(struct bigpc_video_driver *driver) {
  fmn_bcm_swap();
}

const struct bigpc_video_type bigpc_video_type_bcm={
  .name="bcm",
  .desc="Broadcom VideoCore for Raspberry Pi. Newer Pis should prefer DRM.",
  .objlen=sizeof(struct bigpc_video_driver_bcm),
  .del=_bcm_del,
  .init=_bcm_init,
  .begin_gx=_bcm_begin_gx,
  .end=_bcm_end,
};
