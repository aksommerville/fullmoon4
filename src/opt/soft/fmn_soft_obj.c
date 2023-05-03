#include "fmn_soft_internal.h"

//TODO

/* Cleanup.
 */
 
static void _soft_del(struct bigpc_render_driver *driver) {
}

/* Init.
 */
 
static int _soft_init(struct bigpc_render_driver *driver,struct bigpc_video_driver *video) {
  //fprintf(stderr,"%s:%d: TODO soft render\n",__FILE__,__LINE__);
  return -1;
}
 
static int8_t _soft_video_init(
  struct bigpc_render_driver *driver,
  int16_t wmin,int16_t wmax,
  int16_t hmin,int16_t hmax,
  uint8_t pixfmt
) {

  // Prefer (320,192) if in range, next take the nearest in range, then assert gross sanity.
  if (wmin>320) DRIVER->fbw=wmin;
  else if (wmax<320) DRIVER->fbw=wmax;
  else DRIVER->fbw=320;
  if (hmin>192) DRIVER->fbh=hmin;
  else if (hmax<192) DRIVER->fbh=hmax;
  else DRIVER->fbh=192;
  if ((DRIVER->fbw<1)||(DRIVER->fbw>4096)) return -1;
  if ((DRIVER->fbh<1)||(DRIVER->fbh>4096)) return -1;
  
  if (!(DRIVER->pixfmt=fmn_pixfmt_concrete(pixfmt))) return -1;
  
  return 0;
}

static void _soft_video_get_framebuffer_size(int16_t *w,int16_t *h,struct bigpc_render_driver *driver) {
  *w=DRIVER->fbw;
  *h=DRIVER->fbh;
}

static uint8_t _soft_video_get_pixfmt(struct bigpc_render_driver *driver) {
  return DRIVER->pixfmt;
}

static uint32_t _soft_video_rgba_from_pixel(struct bigpc_render_driver *driver,uint32_t pixel) { return pixel; }
static uint32_t _soft_video_pixel_from_rgba(struct bigpc_render_driver *driver,uint32_t rgba) { return rgba; }
static void _soft_video_init_image(struct bigpc_render_driver *driver,uint16_t imageid,int16_t w,int16_t h) {}
static int8_t _soft_draw_set_output(struct bigpc_render_driver *driver,uint16_t imageid) { return 0; }
static void _soft_draw_line(struct bigpc_render_driver *driver,const struct fmn_draw_line *v,int c) {}
static void _soft_draw_rect(struct bigpc_render_driver *driver,const struct fmn_draw_rect *v,int c) {}
static void _soft_draw_mintile(struct bigpc_render_driver *driver,const struct fmn_draw_mintile *v,int c,uint16_t srcimageid) {}
static void _soft_draw_maxtile(struct bigpc_render_driver *driver,const struct fmn_draw_maxtile *v,int c,uint16_t srcimageid) {}
static void _soft_draw_decal(struct bigpc_render_driver *driver,const struct fmn_draw_decal *v,int c,uint16_t srcimageid) {}
static void _soft_draw_decal_swap(struct bigpc_render_driver *driver,const struct fmn_draw_decal *v,int c,uint16_t srcimageid) {}
static void _soft_draw_recal(struct bigpc_render_driver *driver,const struct fmn_draw_recal *v,int c,uint16_t srcimageid) {}
static void _soft_draw_recal_swap(struct bigpc_render_driver *driver,const struct fmn_draw_recal *v,int c,uint16_t srcimageid) {}

static void _soft_begin(struct bigpc_render_driver *driver) {}
static void _soft_end(struct bigpc_render_driver *driver,uint8_t client_result) {}

/* Type.
 */
 
const struct bigpc_render_type bigpc_render_type_soft={
  .name="soft",
  .desc="Pure software rendering, no acceleration.",
  .objlen=sizeof(struct bigpc_render_driver_soft),
  .del=_soft_del,
  .init=_soft_init,
  
  .video_init=_soft_video_init,
  .video_get_framebuffer_size=_soft_video_get_framebuffer_size,
  .video_get_pixfmt=_soft_video_get_pixfmt,
  .video_rgba_from_pixel=_soft_video_rgba_from_pixel,
  .video_pixel_from_rgba=_soft_video_pixel_from_rgba,
  .video_init_image=_soft_video_init_image,
  .draw_set_output=_soft_draw_set_output,
  .draw_line=_soft_draw_line,
  .draw_rect=_soft_draw_rect,
  .draw_mintile=_soft_draw_mintile,
  .draw_maxtile=_soft_draw_maxtile,
  .draw_decal=_soft_draw_decal,
  .draw_decal_swap=_soft_draw_decal_swap,
  .draw_recal=_soft_draw_recal,
  .draw_recal_swap=_soft_draw_recal_swap,
  .begin=_soft_begin,
  .end=_soft_end,
};
