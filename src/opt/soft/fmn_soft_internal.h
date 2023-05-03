#ifndef FMN_SOFT_INTERNAL_H
#define FMN_SOFT_INTERNAL_H

#include "opt/bigpc/bigpc_render.h"
#include "opt/bigpc/bigpc_video.h"
#include "opt/bigpc/bigpc_image.h"
#include <stdio.h>

struct bigpc_render_driver_soft {
  struct bigpc_render_driver hdr;
  int16_t fbw,fbh;
  uint8_t pixfmt;
};

#define DRIVER ((struct bigpc_render_driver_soft*)driver)

#endif
