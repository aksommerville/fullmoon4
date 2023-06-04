#ifndef METAL_INTERNAL_H
#define METAL_INTERNAL_H

#include "opt/bigpc/bigpc_render.h"
#include "opt/macwm/macwm.h"
#include <stdio.h>

struct bigpc_render_driver_metal {
  struct bigpc_render_driver hdr;
  struct macwm *macwm; // WEAK
  int fbw,fbh;
  int pvw,pvh;
  int pixfmt;
};

#define DRIVER ((struct bigpc_render_driver_metal*)driver)

// macwm_bigpc.c
struct macwm *bigpc_video_get_macwm(struct bigpc_video_driver *driver);

#endif
