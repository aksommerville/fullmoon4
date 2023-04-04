#ifndef FMN_GL2_INTERNAL_H
#define FMN_GL2_INTERNAL_H

#include "opt/bigpc/bigpc_render.h"
#include "opt/bigpc/bigpc_video.h"
#include <stdio.h>

struct bigpc_render_driver_gl2 {
  struct bigpc_render_driver hdr;
};

#define DRIVER ((struct bigpc_render_driver_gl2*)driver)

#endif
