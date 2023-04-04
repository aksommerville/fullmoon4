#ifndef FMN_SOFT_INTERNAL_H
#define FMN_SOFT_INTERNAL_H

#include "opt/bigpc/bigpc_render.h"
#include "opt/bigpc/bigpc_video.h"
#include "opt/bigpc/bigpc_image.h"

struct bigpc_render_driver_soft {
  struct bigpc_render_driver hdr;
};

#define DRIVER ((struct bigpc_render_driver_soft*)driver)

#endif
