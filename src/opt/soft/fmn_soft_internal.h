#ifndef FMN_SOFT_INTERNAL_H
#define FMN_SOFT_INTERNAL_H

#include "opt/bigpc/bigpc_render.h"
#include "opt/bigpc/bigpc_video.h"
#include "opt/bigpc/bigpc_image.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

struct bigpc_render_driver_soft {
  struct bigpc_render_driver hdr;
  int fbw,fbh,pixfmt;
  struct soft_image {
    uint16_t id; // id zero is valid; it is the main output
    struct bigpc_image *image;
  } *imagev;
  int imagec,imagea;
  int imagecontigc;
  struct bigpc_image *output; // STRONG; but always one from our (imagev)
  uint16_t outputid;
  int mainpixfmt; // See fmn_soft_obj.c:_soft_begin()
};

#define DRIVER ((struct bigpc_render_driver_soft*)driver)

/* When getting an image, you may supply a callback to create it if needed.
 */
int fmn_soft_image_search(const struct bigpc_render_driver *driver,uint16_t id);
struct bigpc_image *fmn_soft_image_get(
  struct bigpc_render_driver *driver,
  uint16_t id,
  struct bigpc_image *(*cb_create)(void *userdata),
  void *userdata
);

void bigpc_image_convert_in_place(struct bigpc_image *image,int pixfmt);

void bigpc_image_blit(
  struct bigpc_image *dst,int16_t dstx,int16_t dsty,int16_t dstw,int16_t dsth,
  struct bigpc_image *src,int16_t srcx,int16_t srcy,int16_t srcw,int16_t srch,
  uint8_t xform
);

void bigpc_image_blit_rotate(
  struct bigpc_image *dst,int16_t dstx,int16_t dsty,
  struct bigpc_image *src,int16_t srcx,int16_t srcy,int16_t srcw,int16_t srch,
  uint8_t rotate
);

void bigpc_image_blit_recolor(
  struct bigpc_image *dst,int16_t dstx,int16_t dsty,int16_t dstw,int16_t dsth,
  struct bigpc_image *src,int16_t srcx,int16_t srcy,int16_t srcw,int16_t srch,
  uint32_t pixel
);

void bigpc_image_clear(struct bigpc_image *dst);
void bigpc_image_fill_rect(struct bigpc_image *dst,int16_t x,int16_t y,int16_t w,int16_t h,uint32_t pixel);
void bigpc_image_trace_line(struct bigpc_image *dst,int16_t ax,int16_t ay,int16_t bx,int16_t by,uint32_t pixel);

#endif
